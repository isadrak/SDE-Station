#include "web_server.h"

#include <DNSServer.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "../app/watchdog.h"
#include "../config.h"
#include "../web/assets.h"
#include "api.h"
#include "cloud.h"
#include "event_log.h"
#include "history_query.h"
#include "pooled_server.h"
#include "time_keeper.h"

namespace {

PooledWebServer server(HTTP_PORT);
DNSServer dns;  // portail captif : le téléphone ouvre l'appli tout seul
DataStore* store = nullptr;
WifiManager* wifi = nullptr;
String otaError;

void cors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Cache-Control", "no-store");
}

void sendJson(JsonDocument& doc, int code = 200) {
  String body;
  serializeJson(doc, body);
  cors();
  server.send(code, "application/json", body);
}

void sendError(int code, const char* msg) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["msg"] = msg;
  sendJson(doc, code);
}

// Les actions qui effacent ou remplacent quelque chose demandent le mot de
// passe du Wi-Fi de la station.
bool checkPass() {
  if (server.arg("pass") == wifi->apPassword()) return true;
  sendError(403, "Mot de passe de la station incorrect");
  return false;
}

void wifiStatus(JsonObject w) {
  w["ap_ssid"] = wifi->apSsid();
  w["ap_ip"] = wifi->apIp();
  w["ap_clients"] = wifi->apClients();
  w["home_ssid"] = wifi->homeSsid();
  w["home_ok"] = wifi->homeConnected();
  w["home_err"] = wifi->homeError();
  if (wifi->homeConnected()) {
    w["home_ip"] = wifi->homeIp();
    w["rssi"] = wifi->rssi();
  }
  w["hostname"] = WIFI_HOSTNAME ".local";
}

void handleInfo() {
  JsonDocument doc;
  buildFullInfo(doc);
  sendJson(doc);
}

// Les réponses longues sont envoyées par morceaux pour ne pas tout garder
// en RAM.
class ChunkWriter {
 public:
  explicit ChunkWriter(const char* type = "application/json") {
    buf_.reserve(1536);  // un seul tampon pour toute la réponse
    cors();
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, type, "");
  }
  void add(const String& s) { add(s.c_str(), s.length()); }
  void add(const char* s, size_t n) {
    buf_.concat(s, n);
    if (buf_.length() > 1400) flush();
  }
  void end() {
    flush();
    if (!dead_) server.sendContent("");
  }

 private:
  void flush() {
    // Téléphone parti (appli fermée, délai dépassé) : on n'écrit plus, sinon
    // chaque morceau attend jusqu'à ~10 s une lecture qui ne viendra pas.
    if (!dead_ && buf_.length()) {
      if (server.client().connected()) server.sendContent(buf_);
      else dead_ = true;
    }
    buf_ = "";
    wdtPet(WDT_WEB);
  }
  String buf_;
  bool dead_ = false;
};

void handleLive() {
  long since = server.hasArg("since") ? server.arg("since").toInt() : -1;
  ChunkWriter w;
  w.add("{\"up\":" + String(millis() / 1000) + ",\"items\":[");
  bool first = true;
  const size_t n = store->liveCount();
  Sample s;
  JsonDocument doc;
  char line[512];
  for (size_t i = 0; i < n; i++) {
    if (!store->liveAt(i, s)) break;
    if ((long)s.seq <= since) continue;
    doc.clear();
    sampleToJson(s, doc.to<JsonObject>());
    const size_t len = serializeJson(doc, line, sizeof line);
    if (!first) w.add(",", 1);
    w.add(line, len);
    first = false;
  }
  w.add("]}");
  w.end();
}

// ---------------------------------------------------------------- Historique

// GET /api/history?since=<epoch>&until=<epoch>&every=<minutes>
// Lignes : [ts, up, co2, pm25, pm10, t, rh, p, voc, dew] (ts null = heure
// inconnue, utiliser up).
void handleHistory() {
  uint32_t since = server.hasArg("since") ? strtoul(server.arg("since").c_str(), nullptr, 10) : 0;
  uint32_t until = server.hasArg("until") ? strtoul(server.arg("until").c_str(), nullptr, 10) : UINT32_MAX;
  uint32_t every = constrain(server.arg("every").toInt(), 1, 1440);
  if (!server.hasArg("every")) every = 1;

  elog::add("historique demande (pas %u min, tas %u ko)", (unsigned)every, esp_get_free_heap_size() / 1024);
  ChunkWriter w;
  w.add("{\"up\":" + String(millis() / 1000) + ",\"now\":" + String(timekeeper::now()) +
        ",\"cols\":" HISTORY_COLS ",\"rows\":[");
  bool first = true;
  queryHistory(*store, since, until, every, [&](const char* row, size_t len) {
    if (!first) w.add(",", 1);
    w.add(row, len);
    first = false;
  });
  w.add("]}");
  w.end();
}

// « ,valeur » ou « , » si pas de donnée.
int csvVal(char* out, size_t cap, int16_t v, float div, int dec, float offset = 0) {
  if (v == INT16_MIN) return snprintf(out, cap, ",");
  return snprintf(out, cap, ",%.*f", dec, v / div + offset);
}

// Une ligne du CSV dans un tampon fixe (sans String : 43 000 lignes pour 30 jours).
size_t csvRow(const MinuteRec& r, char* out, size_t cap) {
  time_t t = r.ts;
  struct tm tmv;
  gmtime_r(&t, &tmv);
  int n = strftime(out, cap, "%Y-%m-%dT%H:%M:%SZ", &tmv);
  n += csvVal(out + n, cap - n, r.co2, 1, 0);
  n += csvVal(out + n, cap - n, r.pm25, 10, 1);
  n += csvVal(out + n, cap - n, r.pm10, 10, 1);
  n += csvVal(out + n, cap - n, r.tempC, 100, 2);
  n += csvVal(out + n, cap - n, r.rh, 10, 1);
  n += csvVal(out + n, cap - n, r.dew, 10, 1);
  n += csvVal(out + n, cap - n, r.pressure, 10, 1, 900);
  n += snprintf(out + n, cap - n, ",");  // gas (non archivé)
  n += csvVal(out + n, cap - n, r.voc, 10, 1);
  n += snprintf(out + n, cap - n, ",,");  // ah (non archivé)
  localtime_r(&t, &tmv);
  n += strftime(out + n, cap - n, "%Y-%m-%d %H:%M\n", &tmv);
  return n < (int)cap ? n : cap - 1;
}

// Export complet, compatible avec « Importer CSV » de l'appli.
void handleHistoryCsv() {
  server.sendHeader("Content-Disposition", "attachment; filename=\"sde-historique.csv\"");
  ChunkWriter w("text/csv; charset=utf-8");
  w.add("horodatage_iso,co2,pm25,pm10,temp,rh,dew,p,gas,voc,ah,heure_locale\n");
  static MinuteRec buf[64];
  char line[128];
  const uint32_t total = store->flashCount();
  for (uint32_t i = 0; i < total;) {
    size_t got = store->flashRead(i, buf, 64);
    if (!got) break;
    for (size_t k = 0; k < got; k++) w.add(line, csvRow(buf[k], line, sizeof line));
    i += got;
  }
  for (size_t from = 0, n; (n = store->pendingCopy(buf, 64, from)) > 0; from += n)
    for (size_t k = 0; k < n; k++)
      if (buf[k].ts) w.add(line, csvRow(buf[k], line, sizeof line));
  w.end();
}

void handleHistoryReset() {
  if (!checkPass()) return;
  JsonDocument doc;
  doc["ok"] = store->clearHistory();
  doc["msg"] = doc["ok"] ? "Historique efface" : "Erreur de la memoire flash";
  sendJson(doc);
}

// POST /api/reset?mode=restart          redémarre la station
// POST /api/reset?mode=factory&pass=... efface Wi-Fi, mot de passe et historique, puis redémarre
void handleReset() {
  const bool factory = server.arg("mode") == "factory";
  if (factory && !checkPass()) return;
  JsonDocument doc;
  if (factory) {
    store->clearHistory();
    wifi->eraseSettings();
  }
  doc["ok"] = true;
  doc["msg"] = factory ? "Reglages d'usine, redemarrage" : "Redemarrage";
  sendJson(doc);
  delay(500);  // laisse partir la réponse
  ESP.restart();
}

void handleTime() {
  uint32_t epoch = strtoul(server.arg("epoch").c_str(), nullptr, 10);
  JsonDocument doc;
  doc["changed"] = timekeeper::setFromBrowser(epoch);
  doc["src"] = timekeeper::sourceName();
  doc["epoch"] = timekeeper::now();
  sendJson(doc);
}

// ---------------------------------------------------------------- Mise à jour (OTA)

// POST /api/update?pass=... (multipart, fichier « -ota.bin »)
void handleUpdateUpload() {
  HTTPUpload& up = server.upload();
  wdtPet(WDT_WEB);  // l'envoi complet peut durer plus que le watchdog
  if (up.status == UPLOAD_FILE_START) {
    otaError = "";
    if (server.arg("pass") != wifi->apPassword()) {
      otaError = "Mot de passe de la station incorrect";
      return;
    }
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) otaError = Update.errorString();
    elog::add("mise a jour : envoi commence");
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (otaError.isEmpty() && Update.write(up.buf, up.currentSize) != up.currentSize) {
      // Le premier octet d'un firmware est 0xE9 : un « -merged.bin » est refusé ici.
      otaError = Update.errorString();
      Update.abort();
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (otaError.isEmpty() && !Update.end(true)) otaError = Update.errorString();
    elog::add("mise a jour : %s", otaError.isEmpty() ? "installee" : otaError.c_str());
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    otaError = "Envoi interrompu";
  }
}

// GET /api/log : le journal de la station, en texte.
void handleLog() {
  ChunkWriter w("text/plain; charset=utf-8");
  elog::read([](const char* s, size_t n, void* ctx) {
    String part;
    part.concat(s, n);
    static_cast<ChunkWriter*>(ctx)->add(part);
  }, &w);
  w.end();
}

void handleUpdateDone() {
  JsonDocument doc;
  bool ok = otaError.isEmpty() && Update.isFinished();
  doc["ok"] = ok;
  doc["msg"] = ok ? "Mise a jour installee, redemarrage" : (otaError.length() ? otaError : String("Aucun fichier recu"));
  sendJson(doc, ok ? 200 : (otaError.startsWith("Mot de passe") ? 403 : 400));
  if (ok) {
    delay(500);
    ESP.restart();
  }
}

// ---------------------------------------------------------------- Wi-Fi

void handleScan() {
  int n = WiFi.scanNetworks(false, false);
  JsonDocument doc;
  JsonArray arr = doc["networks"].to<JsonArray>();
  for (int i = 0; i < n && arr.size() < 25; i++) {
    // Un même réseau peut apparaître plusieurs fois (répéteurs) : on garde le premier.
    bool dup = false;
    for (JsonObject o : arr) if (o["ssid"] == WiFi.SSID(i)) dup = true;
    if (dup || WiFi.SSID(i).isEmpty()) continue;
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
    o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  sendJson(doc);
}

void handleWifiSet() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.isEmpty() || ssid.length() > 32 || pass.length() > 63) {
    sendError(400, "Nom de reseau ou mot de passe invalide");
    return;
  }
  wifi->setHomeNetwork(ssid, pass);
  JsonDocument doc;
  doc["ok"] = true;
  doc["msg"] = "Connexion en cours";
  sendJson(doc);
}

void handleWifiForget() {
  wifi->forgetHomeNetwork();
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(doc);
}

// Changer le mot de passe demande l'ancien (champ « old »).
void handleApPass() {
  if (server.arg("old") != wifi->apPassword()) {
    sendError(403, "Mot de passe actuel incorrect");
    return;
  }
  String pass = server.arg("pass");
  if (pass.length() < 8 || pass.length() > 63) {
    sendError(400, "Le mot de passe doit faire 8 a 63 caracteres");
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(doc);
  delay(200);  // laisse partir la réponse avant de redémarrer le point d'accès
  wifi->setApPassword(pass);
}

// GET /api/cloud : état de l'accès à distance (jamais le mot de passe)
// POST /api/cloud host=..&user=..&pass=..  (host vide = désactiver ; pass vide = garder)
void handleCloudGet() {
  JsonDocument doc;
  cloud::status(doc.to<JsonObject>());
  sendJson(doc);
}

void handleCloudSet() {
  if (!cloud::configure(server.arg("host"), server.arg("user"), server.arg("pass"))) {
    sendError(400, "Adresse du serveur ou identifiant invalide");
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["msg"] = server.arg("host").length() ? "Connexion au serveur en cours" : "Acces a distance desactive";
  sendJson(doc);
}

void handleCmd() {
  JsonDocument reply;
  runCommand(server.arg("cmd"), reply);
  sendJson(reply);
}

// Page de l'appli (140 ko) envoyée par morceaux, en nourrissant le watchdog :
// avec send_P, un téléphone qui lit lentement ou s'endort au milieu bloquait
// la tâche web plus de 15 s, et le watchdog redémarrait la station. Au-delà
// de 20 s, ou si le client ne lit plus, on abandonne l'envoi.
void sendAsset(const WebAsset* ap) {
  if (ap->gz) server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "no-cache");
  server.setContentLength(ap->len);
  server.send(200, ap->mime, "");
  WiFiClient c = server.client();
  c.setTimeout(2);
  const uint8_t* p = ap->data;
  size_t left = ap->len;
  const uint32_t t0 = millis();
  while (left && c.connected() && millis() - t0 < 20000) {
    wdtPet(WDT_WEB);
    size_t n = c.write(p, left < 2048 ? left : 2048);
    if (!n) break;
    p += n;
    left -= n;
  }
  if (left) c.stop();
}

// Pas sous watchdog : WiFiClient::write réessaie jusqu'à ~10 s par écriture
// quand le téléphone ne lit plus (veille, changement de réseau), et une
// réponse en morceaux en enchaîne plusieurs. Ça redémarrait toute la station.
// wdtPet() garde l'heure du dernier passage pour le diagnostic.
void webTask(void*) {
  for (;;) {
    wdtPet(WDT_WEB);
    dns.processNextRequest();
    server.handleClient();
    wifi->loop();
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

}  // namespace

// État complet (appli) : capteurs, Wi-Fi, historique, accès à distance.
// Servi par /api/info et publié sur le serveur MQTT.
void buildFullInfo(JsonDocument& doc) {
  buildInfo(doc);
  doc["up"] = millis() / 1000;
  doc["heap"] = ESP.getFreeHeap();
  wifiStatus(doc["wifi"].to<JsonObject>());
  JsonObject h = doc["hist"].to<JsonObject>();
  h["flash"] = store->flashOk();
  h["count"] = store->flashCount();
  h["cap"] = DataStore::FLASH_CAPACITY;
  h["fs_used"] = store->fsUsed();
  h["fs_total"] = store->fsTotal();
  if (store->flashCount()) {
    MinuteRec r;
    if (store->flashRead(0, &r, 1)) h["oldest"] = r.ts;
  }
  cloud::status(doc["cloud"].to<JsonObject>());
}

void webServerStart(DataStore* s, WifiManager* w) {
  store = s;
  wifi = w;

  // Fichiers de l'appli (page, manifeste, service worker, icônes)
  for (const WebAsset& a : WEB_ASSETS) {
    const WebAsset* ap = &a;
    server.on(a.path, HTTP_GET, [ap] { sendAsset(ap); });
  }
  server.on("/api/info", HTTP_GET, handleInfo);
  server.on("/api/log", HTTP_GET, handleLog);
  server.on("/api/live", HTTP_GET, handleLive);
  server.on("/api/history", HTTP_GET, handleHistory);
  server.on("/api/history.csv", HTTP_GET, handleHistoryCsv);
  server.on("/api/history/reset", HTTP_POST, handleHistoryReset);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/time", HTTP_POST, handleTime);
  server.on("/api/cmd", HTTP_POST, handleCmd);
  server.on("/api/cloud", HTTP_GET, handleCloudGet);
  server.on("/api/cloud", HTTP_POST, handleCloudSet);
  server.on("/api/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  server.on("/api/wifi/scan", HTTP_GET, handleScan);
  server.on("/api/wifi", HTTP_POST, handleWifiSet);
  server.on("/api/wifi/forget", HTTP_POST, handleWifiForget);
  server.on("/api/wifi/ap", HTTP_POST, handleApPass);
  server.onNotFound([] {
    if (server.method() == HTTP_OPTIONS) {
      cors();
      server.send(204);
      return;
    }
    // Portail captif : toute autre adresse renvoie vers l'appli.
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
    server.send(302);
  });
  server.begin();
  dns.start(53, "*", WiFi.softAPIP());

  xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 2, nullptr, 0);
}
