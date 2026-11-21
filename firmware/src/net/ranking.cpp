#include "ranking.h"

#include <Preferences.h>
#include <freertos/semphr.h>
#include <time.h>

#include "event_log.h"
#include "time_keeper.h"

namespace ranking {
namespace {

constexpr size_t kMaxPeers = 32;     // ~3 ko, réservés seulement si la station participe
constexpr size_t kIdMax = 16;        // « SDE-XXXX »
constexpr size_t kPseudoMax = 41;    // 20 caractères, accents compris (UTF-8), + 0
constexpr int kAirMax = 3;
constexpr uint32_t kDay = 86400;

struct Peer {
  char id[kIdMax];
  char p[kPseudoMax];
  int16_t a, y, w, t;  // ppm ; -1 = pas de valeur
  uint8_t d;
  uint8_t nAir;
  uint16_t s;   // série de bons jours en cours
  uint32_t xp;
  uint32_t ts;
  uint32_t airTs[kAirMax];
  int16_t airDrop[kAirMax];
};

SemaphoreHandle_t mutex_ = nullptr;
Preferences prefs_;
String pseudo_;
char selfId_[kIdMax] = {};
volatile bool dirty_ = false;

Peer* peers_ = nullptr;  // kMaxPeers places, id[0] == 0 = libre
Peer self_ = {};         // dernier résumé de cette station
bool selfOk_ = false;

// Aérations d'aujourd'hui (tâche comms) ; la plus grosse d'abord.
uint32_t airStartTs_ = 0;
float airPeak_ = NAN;
uint32_t airTs_[kAirMax] = {};
int16_t airDrop_[kAirMax] = {};

// Cumuls des jours passés, refaits une fois par jour (le reste de la journée,
// seule la journée en cours est relue en flash).
uint32_t cachedMid_ = 0;
uint64_t sumPast_ = 0, sumWeek_ = 0;
uint32_t nPast_ = 0, nWeek_ = 0;
uint8_t daysPast_ = 0;

// Séries et XP (gardés en NVS) : un « bon jour » = moyenne de CO2 de la
// journée sous kGoodPpm (EN 16798-1 catégorie I, la note 5 du CO2), avec au
// moins 6 h de mesures. Chaque bon jour rapporte 100 XP, multipliés par 2
// toutes les 7 journées de série (x1, x2, x4, puis x8 au plus). Un jour
// au-dessus remet la série à zéro ; un jour sans assez de mesures ne compte pas.
constexpr int kGoodPpm = 970;
uint32_t xp_ = 0;
uint16_t streak_ = 0, best_ = 0;
uint32_t lastDay_ = 0;  // minuit du dernier jour compté

uint32_t dayXp(uint16_t streak) {  // streak = longueur de la série, ce jour compris
  const uint16_t mult = streak > 21 ? 8 : streak > 14 ? 4 : streak > 7 ? 2 : 1;
  return 100u * mult;
}

struct Lock {
  Lock() { xSemaphoreTake(mutex_, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(mutex_); }
};

// Minuit (heure locale de la station) du jour qui contient ts.
uint32_t localMidnight(uint32_t ts) {
  time_t t = ts;
  struct tm lt;
  localtime_r(&t, &lt);
  lt.tm_hour = lt.tm_min = lt.tm_sec = 0;
  lt.tm_isdst = -1;
  return (uint32_t)mktime(&lt);
}

// 2 à 20 caractères, sans caractère de contrôle ni guillemet.
bool validPseudo(const String& p) {
  if (p.length() >= kPseudoMax) return false;
  size_t chars = 0;
  for (size_t i = 0; i < p.length(); i++) {
    const uint8_t c = (uint8_t)p[i];
    if (c < 0x20 || c == 0x7f || c == '"' || c == '\\' || c == '<' || c == '>') return false;
    if ((c & 0xC0) != 0x80) chars++;
  }
  return chars >= 2 && chars <= 20;
}

void ack(JsonDocument& reply, const char* cmd, bool ok, const char* msg) {
  reply["type"] = "ack";
  reply["cmd"] = cmd;
  reply["ok"] = ok;
  reply["msg"] = msg;
}

// Place de la station id dans la table (ou une libre, ou la moins récente).
Peer* slotFor(const char* id) {
  Peer* freeSlot = nullptr;
  Peer* oldest = nullptr;
  for (size_t i = 0; i < kMaxPeers; i++) {
    Peer& q = peers_[i];
    if (q.id[0] && !strcmp(q.id, id)) return &q;
    if (!q.id[0] && !freeSlot) freeSlot = &q;
    if (q.id[0] && (!oldest || q.ts < oldest->ts)) oldest = &q;
  }
  return freeSlot ? freeSlot : oldest;
}

void peerToJson(const Peer& q, JsonObject o) {
  o["id"] = q.id;
  o["p"] = q.p;
  if (q.a >= 0) o["a"] = q.a;
  if (q.y >= 0) o["y"] = q.y;
  if (q.w >= 0) o["w"] = q.w;
  if (q.t >= 0) o["t"] = q.t;
  o["d"] = q.d;
  o["s"] = q.s;
  o["xp"] = q.xp;
  o["ts"] = q.ts;
  JsonArray air = o["air"].to<JsonArray>();
  for (int k = 0; k < q.nAir; k++) {
    JsonArray e = air.add<JsonArray>();
    e.add(q.airTs[k]);
    e.add(q.airDrop[k]);
  }
}

inline int16_t mean(uint64_t sum, uint32_t n) { return n ? (int16_t)((sum + n / 2) / n) : -1; }

}  // namespace

void begin(const String& stationId) {
  strlcpy(selfId_, stationId.c_str(), sizeof selfId_);
  mutex_ = xSemaphoreCreateMutex();
  prefs_.begin("rank", false);
  pseudo_ = prefs_.getString("p", "");
  xp_ = prefs_.getUInt("xp", 0);
  streak_ = prefs_.getUShort("s", 0);
  best_ = prefs_.getUShort("best", 0);
  lastDay_ = prefs_.getUInt("day", 0);
  dirty_ = pseudo_.length() > 0;
}

bool enabled() {
  Lock l;
  return pseudo_.length() > 0;
}

String pseudo() {
  Lock l;
  return pseudo_;
}

bool command(const String& line, JsonDocument& reply) {
  if (line.startsWith("rank.name ")) {
    String p = line.substring(10);
    p.trim();
    if (!validPseudo(p)) {
      ack(reply, "rank.name", false, "Pseudo invalide (2 a 20 caracteres, sans guillemets)");
      return true;
    }
    {
      Lock l;
      pseudo_ = p;
      prefs_.putString("p", p);
    }
    dirty_ = true;
    elog::add("classement : pseudo enregistre");
    ack(reply, "rank.name", true, "Pseudo enregistre : la station rejoint le classement");
    return true;
  }
  if (line == "rank.off") {
    {
      Lock l;
      pseudo_ = "";
      prefs_.remove("p");
      selfOk_ = false;
    }
    dirty_ = true;
    elog::add("classement : station retiree");
    ack(reply, "rank.off", true, "La station a quitte le classement");
    return true;
  }
  return false;
}

void airingStart(float peak) {
  Lock l;
  airStartTs_ = timekeeper::now();
  airPeak_ = peak;
}

void airingEnd(float co2) {
  Lock l;
  if (!airStartTs_ || isnan(airPeak_) || isnan(co2)) return;
  const float drop = airPeak_ - co2;
  const uint32_t start = airStartTs_;
  airStartTs_ = 0;
  if (drop < 100) return;
  const uint32_t mid = localMidnight(start);
  // Oublie les aérations d'un autre jour, puis range la nouvelle par baisse.
  for (int k = 0; k < kAirMax; k++)
    if (airTs_[k] && airTs_[k] < mid) airTs_[k] = 0, airDrop_[k] = 0;
  int slot = -1;
  for (int k = 0; k < kAirMax; k++)
    if (!airTs_[k] || airDrop_[k] < drop) {
      slot = k;
      break;
    }
  if (slot < 0) return;
  for (int k = kAirMax - 1; k > slot; k--) airTs_[k] = airTs_[k - 1], airDrop_[k] = airDrop_[k - 1];
  airTs_[slot] = start;
  airDrop_[slot] = (int16_t)lroundf(drop);
}

bool buildSelf(DataStore& store, JsonDocument& out) {
  const uint32_t now = timekeeper::now();
  if (!now || !enabled()) return false;
  const uint32_t mid = localMidnight(now);
  const uint32_t from = mid - 29 * kDay, weekFrom = mid - 6 * kDay;
  const bool full = mid != cachedMid_;

  static MinuteRec buf[32];  // tâche cloud seulement
  uint64_t sumPast = 0, sumWeek = 0, sumToday = 0;
  uint32_t nPast = 0, nWeek = 0, nToday = 0;
  uint16_t perDay[30] = {};
  uint32_t perSum[30] = {};
  uint32_t idx = store.flashLowerBound(full ? from : mid);
  const uint32_t total = store.flashCount();
  for (uint32_t batch = 0; idx < total; batch++) {
    const size_t n = store.flashRead(idx, buf, 32);
    if (!n) break;
    idx += n;
    for (size_t i = 0; i < n; i++) {
      const MinuteRec& r = buf[i];
      if (!r.ts || r.ts < from || r.co2 == INT16_MIN || r.co2 < 250 || r.co2 > 10000) continue;
      if (r.ts >= mid) {
        sumToday += r.co2;
        nToday++;
      } else {
        sumPast += r.co2;
        nPast++;
        if (r.ts >= weekFrom) sumWeek += r.co2, nWeek++;
        const uint32_t day = (r.ts - from) / kDay;
        if (day < 30 && perDay[day] < 0xFFFF) perDay[day]++, perSum[day] += r.co2;
      }
    }
    if ((batch & 15) == 15) vTaskDelay(1);  // laisse respirer les autres tâches
  }
  if (full) {
    cachedMid_ = mid;
    sumPast_ = sumPast, nPast_ = nPast;
    sumWeek_ = sumWeek, nWeek_ = nWeek;
    daysPast_ = 0;
    for (uint16_t c : perDay)
      if (c >= 360) daysPast_++;
    // Jours terminés pas encore comptés (au plus les 29 en mémoire).
    // Le jour d'index i commence vers from + i jours.
    bool changed = false;
    for (int i = 0; i < 29; i++) {
      const uint32_t dayMid = localMidnight(from + i * kDay + 43200);
      if (dayMid <= lastDay_ || dayMid >= mid) continue;
      lastDay_ = dayMid;
      changed = true;
      if (perDay[i] < 360) continue;
      if ((int)(perSum[i] / perDay[i]) <= kGoodPpm) {
        streak_++;
        if (streak_ > best_) best_ = streak_;
        xp_ += dayXp(streak_);
      } else {
        streak_ = 0;
      }
    }
    if (changed) {
      Lock l;
      prefs_.putUInt("xp", xp_);
      prefs_.putUShort("s", streak_);
      prefs_.putUShort("best", best_);
      prefs_.putUInt("day", lastDay_);
    }
  }
  if (nPast_ + nToday < 60) return false;  // moins d'une heure de mesures

  Peer s = {};
  strlcpy(s.id, selfId_, sizeof s.id);
  strlcpy(s.p, pseudo().c_str(), sizeof s.p);
  s.a = mean(sumPast_ + sumToday, nPast_ + nToday);
  s.y = mean(sumPast_, nPast_);
  s.w = mean(sumWeek_ + sumToday, nWeek_ + nToday);
  s.t = mean(sumToday, nToday);
  s.d = daysPast_ + (nToday >= 360 ? 1 : 0);
  s.ts = now;
  s.s = streak_;
  s.xp = xp_;
  {
    Lock l;
    for (int k = 0; k < kAirMax; k++)
      if (airTs_[k] >= mid) {
        s.airTs[s.nAir] = airTs_[k];
        s.airDrop[s.nAir++] = airDrop_[k];
      }
  }

  out["v"] = 1;
  out["p"] = s.p;
  if (s.a >= 0) out["a"] = s.a;
  if (s.y >= 0) out["y"] = s.y;
  if (s.w >= 0) out["w"] = s.w;
  if (s.t >= 0) out["t"] = s.t;
  out["d"] = s.d;
  out["s"] = s.s;
  out["xp"] = s.xp;
  out["ts"] = s.ts;
  JsonArray air = out["air"].to<JsonArray>();
  for (int k = 0; k < s.nAir; k++) {
    JsonArray e = air.add<JsonArray>();
    e.add(s.airTs[k]);
    e.add(s.airDrop[k]);
  }
  Lock l;
  self_ = s;
  selfOk_ = true;
  return true;
}

bool takeDirty() {
  if (!dirty_) return false;
  dirty_ = false;
  return true;
}

void onPeer(const char* topic, int topicLen, const char* data, int len) {
  // « sde/<id>/classement »
  static const char kSuffix[] = "/classement";
  const int sl = sizeof kSuffix - 1;
  if (topicLen <= 4 + sl || strncmp(topic, "sde/", 4) || strncmp(topic + topicLen - sl, kSuffix, sl)) return;
  const int idLen = topicLen - 4 - sl;
  if (idLen <= 0 || idLen >= (int)kIdMax) return;
  char id[kIdMax];
  memcpy(id, topic + 4, idLen);
  id[idLen] = 0;
  if (strchr(id, '/')) return;

  Lock l;
  if (!pseudo_.length()) return;
  if (!len) {  // la station a quitté le classement
    if (peers_)
      for (size_t i = 0; i < kMaxPeers; i++)
        if (!strcmp(peers_[i].id, id)) peers_[i] = Peer{};
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, data, len) || (doc["v"] | 0) != 1) return;
  const char* p = doc["p"] | "";
  if (!*p || strlen(p) >= kPseudoMax) return;
  if (!peers_) peers_ = (Peer*)calloc(kMaxPeers, sizeof(Peer));
  if (!peers_) return;
  Peer* q = slotFor(id);
  *q = Peer{};
  strlcpy(q->id, id, sizeof q->id);
  strlcpy(q->p, p, sizeof q->p);
  q->a = doc["a"] | -1;
  q->y = doc["y"] | -1;
  q->w = doc["w"] | -1;
  q->t = doc["t"] | -1;
  q->d = doc["d"] | 0;
  q->s = doc["s"] | 0;
  q->xp = doc["xp"] | 0UL;
  q->ts = doc["ts"] | 0UL;
  for (JsonVariant e : doc["air"].as<JsonArray>()) {
    if (q->nAir >= kAirMax) break;
    q->airTs[q->nAir] = e[0] | 0UL;
    q->airDrop[q->nAir++] = e[1] | 0;
  }
}

void clearPeers() {
  Lock l;
  free(peers_);
  peers_ = nullptr;
}

void toJson(JsonDocument& doc) {
  Lock l;
  doc["on"] = pseudo_.length() > 0;
  doc["p"] = pseudo_;
  doc["id"] = selfId_;
  doc["now"] = timekeeper::now();
  doc["best"] = best_;
  doc["good"] = kGoodPpm;
  if (selfOk_) peerToJson(self_, doc["self"].to<JsonObject>());
  JsonArray arr = doc["peers"].to<JsonArray>();
  if (peers_)
    for (size_t i = 0; i < kMaxPeers; i++)
      if (peers_[i].id[0]) peerToJson(peers_[i], arr.add<JsonObject>());
}

void infoJson(JsonObject o) {
  Lock l;
  o["on"] = pseudo_.length() > 0;
  if (pseudo_.length()) o["p"] = pseudo_;
  o["xp"] = xp_;
  o["s"] = streak_;
  o["best"] = best_;
}

}  // namespace ranking
