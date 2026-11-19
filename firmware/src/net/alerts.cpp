#include "alerts.h"

#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>

#include "../config.h"
#include "event_log.h"

namespace alerts {
namespace {

// Racine de Let's Encrypt (ISRG Root X1, expire en 2035), qui signe ntfy.sh.
const char kRootCa[] = R"PEM(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";

struct Msg {
  char title[64];
  char body[160];
  char tags[24];
  uint8_t prio;
};

Preferences prefs;
QueueHandle_t queue;
String id_;
String topic_;
uint16_t co2Max_ = ALERT_CO2_PPM_DEFAULT;
uint8_t rhMax_ = ALERT_RH_DEFAULT;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// État des deux alertes : depuis quand la valeur dépasse, et si l'alerte est
// partie (réarmée quand la valeur redescend sous le seuil moins une marge).
struct Watch {
  uint32_t overSinceMs = 0;
  uint32_t underSinceMs = 0;
  uint32_t lastSentMs = 0;
  bool fired = false;
  bool everSent = false;
};
Watch co2W, rhW;

String topic() {
  portENTER_CRITICAL(&mux);
  String t = topic_;
  portEXIT_CRITICAL(&mux);
  return t;
}

void enqueue(const char* title, const char* body, const char* tags, uint8_t prio) {
  if (!queue) return;
  Msg m;
  strlcpy(m.title, title, sizeof m.title);
  strlcpy(m.body, body, sizeof m.body);
  strlcpy(m.tags, tags, sizeof m.tags);
  m.prio = prio;
  xQueueSend(queue, &m, 0);
}

// Une requête HTTPS courte vers ntfy.sh. La connexion TLS demande ~40 ko de
// tas : on attend que la mémoire le permette (l'accès à distance en prend autant).
bool post(const Msg& m, const String& t) {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (esp_get_free_heap_size() < 60000 || heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < 30000) return false;
  JsonDocument doc;
  doc["topic"] = t;
  doc["title"] = m.title;
  doc["message"] = m.body;
  doc["priority"] = m.prio;
  doc["tags"].to<JsonArray>().add((const char*)m.tags);
  String payload;
  serializeJson(doc, payload);

  WiFiClientSecure c;
  c.setCACert(kRootCa);
  c.setTimeout(10);
  if (!c.connect(ALERT_NTFY_HOST, 443)) {
    elog::add("alerte : connexion a %s impossible", ALERT_NTFY_HOST);
    return false;
  }
  c.printf("POST / HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %u\r\nConnection: close\r\n\r\n",
           ALERT_NTFY_HOST, (unsigned)payload.length());
  c.print(payload);
  const String status = c.readStringUntil('\n');
  c.stop();
  const bool ok = status.startsWith("HTTP/1.1 200");
  if (ok) elog::add("alerte envoyee : %s", m.title);
  else elog::add("alerte refusee : %s", status.c_str());
  return ok;
}

// Tâche d'envoi : bas niveau de priorité, pile assez grande pour mbedTLS.
// Un message qui n'a pas pu partir est réessayé toutes les 5 min pendant 1 h.
void task(void*) {
  Msg m;
  for (;;) {
    if (xQueueReceive(queue, &m, portMAX_DELAY) != pdTRUE) continue;
    const String t = topic();
    if (!t.length()) continue;
    for (int tries = 0; tries < 13; tries++) {
      if (post(m, t)) break;
      vTaskDelay(pdMS_TO_TICKS(5 * 60 * 1000));
    }
  }
}

// over : la valeur dépasse le seuil ; under : elle est repassée sous le seuil
// moins la marge. Retourne true quand l'alerte doit partir.
bool watch(Watch& w, bool valid, bool over, bool under, uint32_t holdMs, uint32_t rearmMs, uint32_t minGapMs) {
  const uint32_t now = millis();
  if (!valid) return false;
  if (over) {
    if (!w.overSinceMs) w.overSinceMs = now;
    w.underSinceMs = 0;
  } else {
    w.overSinceMs = 0;
    if (under) {
      if (!w.underSinceMs) w.underSinceMs = now;
      if (w.fired && now - w.underSinceMs >= rearmMs) w.fired = false;
    } else {
      w.underSinceMs = 0;
    }
  }
  if (w.fired || !w.overSinceMs || now - w.overSinceMs < holdMs) return false;
  if (w.everSent && now - w.lastSentMs < minGapMs) return false;
  w.fired = true;
  w.everSent = true;
  w.lastSentMs = now;
  return true;
}

String where(const String& room) { return room.length() ? room : id_; }

}  // namespace

void begin(const String& stationId) {
  id_ = stationId;
  prefs.begin("alert", false);
  topic_ = prefs.getString("topic", "");
  co2Max_ = prefs.getUShort("co2", ALERT_CO2_PPM_DEFAULT);
  rhMax_ = prefs.getUChar("rh", ALERT_RH_DEFAULT);
  queue = xQueueCreate(4, sizeof(Msg));
  xTaskCreatePinnedToCore(task, "alerts", 8192, nullptr, 1, nullptr, 0);
}

void onSample(const Sample& s, const String& room) {
  if (!topic().length()) return;
  char title[64], body[160];
  const float co2 = s.co2Avg;
  const bool co2Valid = s.co2Ok && !s.co2Warm && !isnan(co2);
  if (watch(co2W, co2Valid, co2 >= co2Max_, co2 < co2Max_ - 100, 10UL * 60000, 10UL * 60000, 3UL * 3600000)) {
    snprintf(title, sizeof title, "CO2 eleve : %s", where(room).c_str());
    snprintf(body, sizeof body, "CO2 a %.0f ppm depuis 10 min (seuil %u). Il est temps d'aerer la piece.", co2,
             (unsigned)co2Max_);
    enqueue(title, body, "warning", 4);
  }
  if (watch(rhW, s.envOk, s.rh >= rhMax_, s.rh < rhMax_ - 3, 3UL * 3600000, 60UL * 60000, 12UL * 3600000)) {
    snprintf(title, sizeof title, "Air humide : %s", where(room).c_str());
    snprintf(body, sizeof body,
             "Humidite a %.0f %% depuis 3 h (seuil %u %%). Risque de moisissure : aerer, chauffer un peu ou deshumidifier.",
             s.rh, (unsigned)rhMax_);
    enqueue(title, body, "droplet", 3);
  }
}

static void ack(JsonDocument& r, const char* cmd, bool ok, const char* msg) {
  r["type"] = "ack";
  r["cmd"] = cmd;
  r["ok"] = ok;
  r["msg"] = msg;
}

bool command(const String& line, JsonDocument& reply) {
  if (line.startsWith("alert.topic ")) {
    String t = line.substring(12);
    t.trim();
    bool valid = t.length() >= 8 && t.length() <= 64;
    for (unsigned i = 0; i < t.length(); i++) {
      const char ch = t[i];
      valid = valid && (isalnum((unsigned char)ch) || ch == '-' || ch == '_');
    }
    if (!valid) {
      ack(reply, "alert.topic", false, "Sujet invalide (8 a 64 caracteres : lettres, chiffres, - ou _)");
      return true;
    }
    prefs.putString("topic", t);
    portENTER_CRITICAL(&mux);
    topic_ = t;
    portEXIT_CRITICAL(&mux);
    co2W = Watch();
    rhW = Watch();
    ack(reply, "alert.topic", true, "Alertes activees");
  } else if (line == "alert.off") {
    prefs.putString("topic", "");
    portENTER_CRITICAL(&mux);
    topic_ = "";
    portEXIT_CRITICAL(&mux);
    ack(reply, "alert.off", true, "Alertes coupees");
  } else if (line.startsWith("alert.co2 ")) {
    const long v = line.substring(10).toInt();
    if (v < 600 || v > 5000) {
      ack(reply, "alert.co2", false, "Valeur hors limites (600 a 5000 ppm)");
      return true;
    }
    co2Max_ = (uint16_t)v;
    prefs.putUShort("co2", co2Max_);
    co2W = Watch();
    ack(reply, "alert.co2", true, "Seuil de CO2 enregistre");
  } else if (line.startsWith("alert.rh ")) {
    const long v = line.substring(9).toInt();
    if (v < 40 || v > 90) {
      ack(reply, "alert.rh", false, "Valeur hors limites (40 a 90 %)");
      return true;
    }
    rhMax_ = (uint8_t)v;
    prefs.putUChar("rh", rhMax_);
    rhW = Watch();
    ack(reply, "alert.rh", true, "Seuil d'humidite enregistre");
  } else if (line == "alert.test") {
    if (!topic().length()) {
      ack(reply, "alert.test", false, "Alertes coupees : choisir d'abord un sujet");
      return true;
    }
    enqueue((String("Test : ") + id_).c_str(), "Les alertes de la station arrivent bien sur ce telephone.", "white_check_mark", 3);
    ack(reply, "alert.test", true, "Alerte de test envoyee (si la station est sur le Wi-Fi de la maison)");
  } else {
    return false;
  }
  return true;
}

void toJson(JsonObject o) {
  const String t = topic();
  o["on"] = t.length() > 0;
  if (t.length()) o["topic"] = t;
  o["server"] = ALERT_NTFY_HOST;
  o["co2"] = co2Max_;
  o["rh"] = rhMax_;
}

}  // namespace alerts
