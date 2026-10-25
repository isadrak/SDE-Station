#include "cloud.h"

#include <Preferences.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_tls_errors.h>
#include <mqtt_client.h>

#include "../config.h"
#include "api.h"
#include "event_log.h"
#include "history_query.h"
#include "time_keeper.h"

namespace cloud {
namespace {

// Autorités acceptées pour le certificat du serveur EMQX Cloud (Serverless) :
//   Encryption Everywhere DV TLS CA - G2, l'intermédiaire DigiCert qui signe
//   le certificat d'EMQX et que le serveur n'envoie pas (expire le
//   2027-11-27 ; à remplacer si EMQX change d'intermédiaire), puis sa
//   racine DigiCert Global Root G2, pour un autre intermédiaire qu'EMQX
//   enverrait lui-même.
// Chaque certificat de la liste reste décodé en mémoire pendant toute la
// connexion (environ 1,5 ko chacun) : la liste reste la plus courte possible.
const char kRootCa[] = R"PEM(-----BEGIN CERTIFICATE-----
MIIEqjCCA5KgAwIBAgIQDeD/te5iy2EQn2CMnO1e0zANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xNzExMjcxMjQ2NDBaFw0yNzExMjcxMjQ2NDBaMG4xCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xLTArBgNVBAMTJEVuY3J5cHRpb24gRXZlcnl3aGVyZSBEViBUTFMgQ0EgLSBH
MjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAO8Uf46i/nr7pkgTDqnE
eSIfCFqvPnUq3aF1tMJ5hh9MnO6Lmt5UdHfBGwC9Si+XjK12cjZgxObsL6Rg1njv
NhAMJ4JunN0JGGRJGSevbJsA3sc68nbPQzuKp5Jc8vpryp2mts38pSCXorPR+sch
QisKA7OSQ1MjcFN0d7tbrceWFNbzgL2csJVQeogOBGSe/KZEIZw6gXLKeFe7mupn
NYJROi2iC11+HuF79iAttMc32Cv6UOxixY/3ZV+LzpLnklFq98XORgwkIJL1HuvP
ha8yvb+W6JislZJL+HLFtidoxmI7Qm3ZyIV66W533DsGFimFJkz3y0GeHWuSVMbI
lfsCAwEAAaOCAU8wggFLMB0GA1UdDgQWBBR435GQX+7erPbFdevVTFVT7yRKtjAf
BgNVHSMEGDAWgBROIlQgGJXm427mD/r6uRLtBhePOTAOBgNVHQ8BAf8EBAMCAYYw
HQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8C
AQAwNAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdp
Y2VydC5jb20wQgYDVR0fBDswOTA3oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQu
Y29tL0RpZ2lDZXJ0R2xvYmFsUm9vdEcyLmNybDBMBgNVHSAERTBDMDcGCWCGSAGG
/WwBAjAqMCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BT
MAgGBmeBDAECATANBgkqhkiG9w0BAQsFAAOCAQEAoBs1eCLKakLtVRPFRjBIJ9LJ
L0s8ZWum8U8/1TMVkQMBn+CPb5xnCD0GSA6L/V0ZFrMNqBirrr5B241OesECvxIi
98bZ90h9+q/X5eMyOD35f8YTaEMpdnQCnawIwiHx06/0BfiTj+b/XQih+mqt3ZXe
xNCJqKexdiB2IWGSKcgahPacWkk/BAQFisKIFYEqHzV974S3FAz/8LIfD58xnsEN
GfzyIDkH3JrwYZ8caPTf6ZX9M1GrISN8HnWTtdNCH2xEajRa/h9ZBXjUyFKQrGk2
n2hcLrfZSbynEC/pSw/ET7H5nWwckjmAJ1l9fcnbqkU/pf6uMQmnfl0JQjJNSg==
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)PEM";

constexpr size_t kReqMax = 384;
struct Request {
  char text[kReqMax];
};

DataStore* store_ = nullptr;
WifiManager* wifi_ = nullptr;
Preferences prefs_;
SemaphoreHandle_t mutex_;   // réglages (écrits par le serveur web)
QueueHandle_t reqQueue_;    // demandes reçues, traitées par la tâche cloud
QueueHandle_t sampleBox_;   // une place : la dernière mesure

String host_, user_, pass_;
String base_;               // "sde/SDE-XXXX/"
volatile bool dirty_ = false;      // réglages changés : relancer le client
esp_mqtt_client_handle_t client_ = nullptr;
volatile bool connected_ = false;
volatile uint32_t droppedAt_ = 0;      // connexion perdue ou ratée : client à détruire
volatile bool wasUp_ = false;          // ce client s'est connecté au moins une fois
volatile bool needInfo_ = false, needLive_ = false;
const char* volatile err_ = nullptr;   // dernière erreur, en clair
volatile bool paused_ = false;         // coupé au démarrage après des plantages
char errBuf_[160];

// Traduit l'échec de connexion TCP/TLS, avec les codes bruts à la fin pour
// le diagnostic (esp-tls, mbedtls, drapeaux du certificat).
const char* describeTransportError(const esp_mqtt_error_codes_t& h) {
  const esp_err_t tls = h.esp_tls_last_esp_err;
  const int flags = h.esp_tls_cert_verify_flags;
  const char* what;
  if (tls == ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME) what = "Adresse du serveur introuvable (verifie l'adresse)";
  else if (tls == ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST || tls == ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET)
    what = "Le serveur ne repond pas sur le port 8883 (reseau qui le bloque ?)";
  else if (tls == ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT) what = "Delai depasse en joignant le serveur";
  else if (flags & 0x08) what = "Certificat du serveur non reconnu par la station";
  else if (flags) what = "Certificat du serveur refuse";
  else if (tls == ESP_ERR_MBEDTLS_SSL_HANDSHAKE_FAILED) what = "Echec de la connexion securisee (TLS)";
  else what = "Serveur injoignable (adresse, ou pas d'Internet)";
  snprintf(errBuf_, sizeof errBuf_, "%s [%x/%x/%x/%d, tas %u ko, bloc %u ko]", what, (unsigned)tls,
           (unsigned)-h.esp_tls_stack_err, (unsigned)flags, h.esp_transport_sock_errno,
           (unsigned)(esp_get_free_heap_size() / 1024), (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024));
  return errBuf_;
}
uint32_t watchMs_ = 0;                 // dernière demande d'une appli
bool watched_ = false;

// Même erreur à chaque nouvel essai : une seule ligne dans le journal.
void logError() {
  static const char* last = nullptr;
  static char lastText[40];  // sans les chiffres de mémoire qui suivent
  const char* e = err_;
  if (!e || (e == last && !strncmp(e, lastText, sizeof lastText - 1))) return;
  last = e;
  strlcpy(lastText, e, sizeof lastText);
  elog::add("acces a distance : %s", e);
}

void onEvent(void*, esp_event_base_t, int32_t id, void* data) {
  auto* e = static_cast<esp_mqtt_event_handle_t>(data);
  switch ((esp_mqtt_event_id_t)id) {
    case MQTT_EVENT_CONNECTED:
      connected_ = true;
      wasUp_ = true;
      err_ = nullptr;
      elog::add("acces a distance : connecte (tas %u ko)", esp_get_free_heap_size() / 1024);
      esp_mqtt_client_subscribe(client_, (base_ + "req").c_str(), 0);
      esp_mqtt_client_publish(client_, (base_ + "status").c_str(), "online", 0, 1, 1);
      needInfo_ = needLive_ = true;
      break;
    case MQTT_EVENT_DISCONNECTED:
      if (connected_) elog::add("acces a distance : deconnecte");
      connected_ = false;
      droppedAt_ = millis() | 1;
      break;
    case MQTT_EVENT_DATA:
      // Les demandes tiennent toujours en un seul morceau.
      if (e->current_data_offset == 0 && e->data_len == e->total_data_len && e->data_len < (int)kReqMax) {
        Request r;
        memcpy(r.text, e->data, e->data_len);
        r.text[e->data_len] = 0;
        xQueueSend(reqQueue_, &r, 0);
      }
      break;
    case MQTT_EVENT_ERROR:
      if (e->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
        const auto rc = e->error_handle->connect_return_code;
        err_ = (rc == MQTT_CONNECTION_REFUSE_BAD_USERNAME || rc == MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED)
                   ? "Identifiant ou mot de passe refuse par le serveur"
                   : "Connexion refusee par le serveur";
      } else {
        err_ = describeTransportError(*e->error_handle);
      }
      logError();
      droppedAt_ = millis() | 1;
      break;
    default:
      break;
  }
}

void startClient() {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  const String uri = "mqtts://" + host_ + ":" + String(CLOUD_MQTT_PORT);
  const String user = user_, pass = pass_;
  xSemaphoreGive(mutex_);
  const String lwt = base_ + "status";
  const String id = wifi_->apSsid();

  esp_mqtt_client_config_t cfg = {};
  cfg.uri = uri.c_str();
  cfg.username = user.c_str();
  cfg.password = pass.c_str();
  cfg.client_id = id.c_str();
  cfg.cert_pem = kRootCa;
  cfg.lwt_topic = lwt.c_str();
  cfg.lwt_msg = "offline";
  cfg.lwt_qos = 1;
  cfg.lwt_retain = 1;
  cfg.keepalive = 60;
  // Un message plus long que le tampon de sortie part en plusieurs écritures
  // (QoS 0) : inutile de garder un tampon de la taille d'un morceau.
  cfg.buffer_size = 1024;
  cfg.out_buffer_size = 1024;
  cfg.task_stack = 6144;
  cfg.task_prio = 2;  // sous la tâche comms : la poignée de main TLS occupe le cœur ~1 s
  // Pas de nouvel essai automatique : chaque essai réserve ~40 ko pour le
  // TLS. La tâche cloud détruit le client et réessaie elle-même, de plus en
  // plus espacé, seulement s'il y a assez de mémoire (voir task()).
  cfg.disable_auto_reconnect = true;
  cfg.network_timeout_ms = 10000;
  // Le client copie ces chaînes : les String locales peuvent disparaître.
  client_ = esp_mqtt_client_init(&cfg);
  if (!client_) {
    err_ = "Memoire insuffisante pour l'acces a distance";
    return;
  }
  droppedAt_ = 0;
  wasUp_ = false;
  esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY, onEvent, nullptr);
  esp_mqtt_client_start(client_);
}

void stopClient() {
  if (!client_) return;
  if (connected_) esp_mqtt_client_publish(client_, (base_ + "status").c_str(), "offline", 0, 0, 1);
  esp_mqtt_client_stop(client_);
  esp_mqtt_client_destroy(client_);
  client_ = nullptr;
  connected_ = false;
}

void publish(const String& topic, const String& payload, bool retain) {
  esp_mqtt_client_publish(client_, topic.c_str(), payload.c_str(), payload.length(), 0, retain ? 1 : 0);
}

void publishJson(const String& topic, JsonDocument& doc, bool retain) {
  String s;
  serializeJson(doc, s);
  publish(topic, s, retain);
}

// Mémoire minimale pour continuer un envoi : en dessous, le Wi-Fi et le
// serveur web de la station manqueraient de tampons.
constexpr size_t kSendHeapMin = 30000;
constexpr size_t kCutHeap = 20000;  // en dessous pendant 20 s : client MQTT coupé

bool memoryOk() { return esp_get_free_heap_size() >= kSendHeapMin; }

// L'historique part en morceaux de CLOUD_CHUNK octets, construits sur place
// dans un seul tampon réservé d'avance (pas de copies ni de mémoire hachée),
// à ~10 ko/s. S'arrête si la connexion tombe ou si la mémoire baisse : l'appli
// reçoit alors une erreur au lieu d'une station bloquée.
void sendHistory(const String& topic, uint32_t id, JsonDocument& req) {
  const uint32_t since = req["since"] | 0UL;
  const uint32_t until = req["until"] | 0xFFFFFFFFUL;
  const uint32_t every = constrain((long)(req["every"] | 1L), 1L, 1440L);
  const String head = "{\"id\":" + String(id) + ",\"rows\":[";
  String msg;
  if (!msg.reserve(CLOUD_CHUNK + 256)) return;
  msg = head;
  bool ok = true, empty = true;
  uint32_t rows = 0, chunks = 0;
  const uint32_t t0 = millis();
  auto send = [&](bool done) {
    msg += ']';
    if (done)
      msg += ",\"done\":true,\"up\":" + String(millis() / 1000) + ",\"now\":" + String(timekeeper::now()) +
             ",\"cols\":" HISTORY_COLS;
    msg += '}';
    if (esp_mqtt_client_publish(client_, topic.c_str(), msg.c_str(), msg.length(), 0, 0) < 0) ok = false;
    chunks++;
    msg = head;
    empty = true;
    vTaskDelay(pdMS_TO_TICKS(200));  // EMQX Serverless : 10 publications/s au plus
  };
  queryHistory(*store_, since, until, every, [&](const char* row, size_t len) {
    if (!ok) return;
    if (!connected_ || !memoryOk()) {
      ok = false;
      return;
    }
    if (++rows % 64 == 0) vTaskDelay(1);  // laisse respirer le cœur 0 (Wi-Fi, IDLE)
    if (!empty) msg += ',';
    msg.concat(row, len);
    empty = false;
    if (msg.length() > CLOUD_CHUNK) send(false);
  });
  if (ok && connected_) send(true);
  else {
    JsonDocument res;  // l'appli abandonne tout de suite au lieu d'attendre
    res["id"] = id;
    res["ok"] = false;
    res["done"] = true;
    res["msg"] = memoryOk() ? "Connexion perdue pendant l'envoi" : "Memoire insuffisante dans la station";
    publishJson(topic, res, false);
  }
  elog::add("acces a distance : historique %s, %u lignes en %u morceaux, %u ms, tas %u ko", ok ? "envoye" : "interrompu",
            (unsigned)rows, (unsigned)chunks, (unsigned)(millis() - t0), esp_get_free_heap_size() / 1024);
}

void sendLog(const String& topic, uint32_t id) {
  JsonDocument res;
  res["id"] = id;
  res["log"] = elog::tail(2000);
  publishJson(topic, res, false);
}

void handleRequest(const char* text) {
  JsonDocument req;
  if (deserializeJson(req, text)) return;
  const char* c = req["c"] | "";
  if (!*c || strlen(c) > 40 || strpbrk(c, "/+#")) return;
  const String topic = base_ + "res/" + c;
  const uint32_t id = req["id"] | 0UL;
  const String op = req["op"] | "";
  watchMs_ = millis();
  if (!watched_) needLive_ = true;
  watched_ = true;

  if (op == "watch") return;
  if (op.length()) elog::add("acces a distance : demande %s (tas %u ko)", op.c_str(), esp_get_free_heap_size() / 1024);
  if (op == "log") {
    sendLog(topic, id);
    return;
  }
  if (op == "history") {
    sendHistory(topic, id, req);
    return;
  }
  JsonDocument res;
  if (op == "cmd") {
    const String cmd = req["cmd"] | "";
    if (cmd.startsWith("wifi")) {
      // Changer de réseau à distance couperait la station d'Internet.
      res["type"] = "ack";
      res["cmd"] = cmd;
      res["ok"] = false;
      res["msg"] = "Le Wi-Fi se regle seulement sur place";
    } else {
      runCommand(cmd, res);
    }
  } else if (op == "info") {
    buildFullInfo(res);
  } else {
    res["ok"] = false;
    res["msg"] = "Demande inconnue";
  }
  res["id"] = id;
  publishJson(topic, res, false);
}

// Pas de watchdog sur cette tâche : une publication attend que le client MQTT
// ait fini un essai de connexion (DNS, TLS, jusqu'à ~15 s). Si le réseau
// coince, seul l'accès à distance attend ; les mesures et le Wi-Fi continuent.
void task(void*) {
  uint32_t lastInfo = 0, lastLive = 0, nextTry = 0, lowSince = 0;
  uint8_t fails = 0;
  for (;;) {
    if (dirty_) {
      dirty_ = false;
      stopClient();
      fails = 0;
      nextTry = millis();
    }
    // Connexion perdue ou ratée : on libère tout (TLS, tampons, tâche MQTT)
    // et on réessaie dans 15 s, 30 s, 1, 2 puis 5 min. On laisse 2 s à la
    // tâche MQTT pour se terminer d'elle-même avant de détruire le client.
    if (client_ && droppedAt_ && millis() - droppedAt_ > 2000) {
      if (wasUp_) fails = 0;  // elle avait marché : on recommence vite
      stopClient();
      static const uint16_t kWaitS[] = {15, 30, 60, 120, 300};
      nextTry = millis() + kWaitS[fails < 4 ? fails : 4] * 1000UL;
      if (fails < 255) fails++;
    }
    // On attend Internet et l'heure (NTP) avant de lancer le client ; au
    // pire 60 s après le démarrage.
    if (!client_ && !paused_ && host_.length() && wifi_->homeConnected() && (timekeeper::valid() || millis() > 60000) &&
        (int32_t)(millis() - nextTry) >= 0) {
      // Le TLS demande d'un coup un bloc de 16 ko et environ 40 ko en tout :
      // sans cette marge, l'allocation échoue ailleurs et la station plante.
      const size_t heap = esp_get_free_heap_size(), block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
      if (heap < 60000 || block < 20000) {
        snprintf(errBuf_, sizeof errBuf_, "Memoire insuffisante pour l'acces a distance (tas %u ko, bloc %u ko)",
                 (unsigned)(heap / 1024), (unsigned)(block / 1024));
        err_ = errBuf_;
        logError();
        nextTry = millis() + 30000;
      } else {
        startClient();
      }
    }

    Request r;
    if (xQueueReceive(reqQueue_, &r, pdMS_TO_TICKS(100)) == pdTRUE && connected_) handleRequest(r.text);

    // Garde-fou : si la mémoire reste trop basse, on coupe l'accès à distance
    // (le TLS rend ~40 ko) pour garder la station joignable sur place.
    const uint32_t now = millis();
    if (client_ && esp_get_free_heap_size() < kCutHeap) {
      if (!lowSince) lowSince = now | 1;
      else if (now - lowSince > 20000) {
        snprintf(errBuf_, sizeof errBuf_, "Coupe 5 min : memoire trop basse (tas %u ko)",
                 (unsigned)(esp_get_free_heap_size() / 1024));
        err_ = errBuf_;
        logError();
        stopClient();
        nextTry = now + 300000;
        lowSince = 0;
        continue;
      }
    } else {
      lowSince = 0;
    }
    if (!connected_) continue;

    if (watched_ && now - watchMs_ > CLOUD_WATCH_MS) watched_ = false;
    if (needInfo_ || now - lastInfo > CLOUD_INFO_MS) {
      needInfo_ = false;
      lastInfo = now;
      JsonDocument doc;
      buildFullInfo(doc);
      publishJson(base_ + "info", doc, true);
    }
    Sample s;
    if (xQueueReceive(sampleBox_, &s, 0) == pdTRUE) {
      // Les particules n'arrivent qu'une fois par cycle : jamais sautées.
      if (watched_ || needLive_ || s.pmFresh || now - lastLive > CLOUD_LIVE_IDLE_MS) {
        needLive_ = false;
        lastLive = now;
        JsonDocument doc;
        sampleToJson(s, doc.to<JsonObject>());
        publishJson(base_ + "live", doc, true);
      }
    }
  }
}

// « xxxxxxxx.ala.eu-central-1.emqxsl.com », même collé avec mqtts://, wss://, un port ou /mqtt.
String cleanHost(String h) {
  h.trim();
  int i = h.indexOf("://");
  if (i >= 0) h = h.substring(i + 3);
  i = h.indexOf('/');
  if (i >= 0) h = h.substring(0, i);
  i = h.indexOf(':');
  if (i >= 0) h = h.substring(0, i);
  h.toLowerCase();
  return h;
}

}  // namespace

void begin(DataStore* store, WifiManager* wifi, bool paused) {
  store_ = store;
  wifi_ = wifi;
  mutex_ = xSemaphoreCreateMutex();
  reqQueue_ = xQueueCreate(4, sizeof(Request));
  sampleBox_ = xQueueCreate(1, sizeof(Sample));
  base_ = "sde/" + wifi->apSsid() + "/";
  prefs_.begin("cloud", false);
  host_ = prefs_.getString("host", "");
  user_ = prefs_.getString("user", "");
  pass_ = prefs_.getString("pass", "");
  paused_ = paused && host_.length();
  if (paused_) err_ = "En pause apres plusieurs plantages : reenregistre pour reessayer";
  // Erreurs et avertissements du client MQTT et du TLS : dans le journal de
  // la station (event_log.cpp les retire de l'USB).
  for (const char* tag : {"MQTT_CLIENT", "TRANSPORT_BASE", "TRANSPORT", "TRANS_SSL", "TRANS_TCP", "esp-tls",
                          "esp-tls-mbedtls", "OUTBOX"})
    esp_log_level_set(tag, ESP_LOG_WARN);
  xTaskCreatePinnedToCore(task, "cloud", 8192, nullptr, 2, nullptr, 0);
}

void onSample(const Sample& s) {
  if (sampleBox_) xQueueOverwrite(sampleBox_, &s);
}

bool configure(const String& host, const String& user, const String& pass) {
  const String h = cleanHost(host);
  if (h.length() > 100 || user.length() > 64 || pass.length() > 64) return false;
  if (h.length() && (h.indexOf('.') < 0 || !user.length())) return false;
  xSemaphoreTake(mutex_, portMAX_DELAY);
  host_ = h;
  user_ = h.length() ? user : String();
  // Mot de passe vide = garder l'actuel (pour changer seulement l'adresse).
  if (!h.length()) pass_ = "";
  else if (pass.length()) pass_ = pass;
  prefs_.putString("host", host_);
  prefs_.putString("user", user_);
  prefs_.putString("pass", pass_);
  err_ = nullptr;
  paused_ = false;
  xSemaphoreGive(mutex_);
  dirty_ = true;
  return true;
}

void status(JsonObject out) {
  xSemaphoreTake(mutex_, portMAX_DELAY);
  out["on"] = host_.length() > 0;
  if (host_.length()) {
    out["host"] = host_;
    out["user"] = user_;
  }
  xSemaphoreGive(mutex_);
  out["ok"] = (bool)connected_;
  out["topic"] = base_;
  if (err_ && !connected_) out["err"] = (const char*)err_;
}

}  // namespace cloud
