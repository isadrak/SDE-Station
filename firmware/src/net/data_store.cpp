#include "data_store.h"

#include <LittleFS.h>

#include "time_keeper.h"

namespace {
enum Field { F_CO2, F_T, F_RH, F_P, F_VOC, F_DEW };

const char* HIST_DIR = "/h";
const size_t REC = sizeof(MinuteRec);

int16_t scaled(double sum, uint16_t n, float mul, float offset = 0) {
  if (n == 0) return INT16_MIN;
  double v = (sum / n - offset) * mul;
  return (int16_t)constrain(lround(v), -32767L, 32767L);
}
int16_t scaledF(float v, float mul) {
  if (isnan(v)) return INT16_MIN;
  return (int16_t)constrain(lroundf(v * mul), -32767L, 32767L);
}

int16_t pack(float v, float mul, float offset = 0) {
  if (isnan(v)) return INT16_MIN;
  return (int16_t)constrain(lroundf((v - offset) * mul), -32767L, 32767L);
}
float unpack(int16_t v, float div, float offset = 0) { return v == INT16_MIN ? NAN : v / div + offset; }

enum : uint8_t { L_CO2_OK = 1, L_CO2_WARM = 2, L_CO2_SWAP = 4, L_PM_OK = 8, L_PM_FRESH = 16, L_ENV_OK = 32 };

struct Lock {
  explicit Lock(SemaphoreHandle_t m) : m_(m) { xSemaphoreTake(m_, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(m_); }
  SemaphoreHandle_t m_;
};
}  // namespace

// Point en direct à la précision du JSON envoyé (sampleToJson arrondit déjà
// au dixième, au centième pour la température).
struct DataStore::LiveRec {
  uint32_t seq, up;
  float gas;
  uint16_t co2Rx, pmPeriodS, pmNextS, co2Avg;  // co2Avg x10, 0xFFFF = rien
  int16_t co2, pm25, pm10, tempC, rh, pressure, voc, dew, ah;
  int8_t co2TempC;
  uint8_t envAddr, pmState, flags;

  void set(const Sample& s) {
    seq = s.seq;
    up = s.uptimeS;
    gas = s.gasKohm;
    co2Rx = s.co2Rx;
    pmPeriodS = (uint16_t)min<uint32_t>(s.pmPeriodS, 65535);
    pmNextS = (uint16_t)min<uint32_t>(s.pmNextS, 65535);
    co2Avg = isnan(s.co2Avg) ? 0xFFFF : (uint16_t)constrain(lroundf(s.co2Avg * 10), 0L, 65534L);
    co2 = pack(s.co2, 1);
    pm25 = pack(s.pm25, 10);
    pm10 = pack(s.pm10, 10);
    tempC = pack(s.tempC, 100);
    rh = pack(s.rh, 10);
    pressure = pack(s.pressure, 10, 900);
    voc = pack(s.vocIdx, 10);
    dew = pack(s.dewPoint, 10);
    ah = pack(s.absHum, 10);
    co2TempC = (int8_t)constrain(lroundf(s.co2TempC), -128L, 127L);
    envAddr = s.envAddr;
    pmState = s.pmState;
    flags = (s.co2Ok ? L_CO2_OK : 0) | (s.co2Warm ? L_CO2_WARM : 0) | (s.co2Swapped ? L_CO2_SWAP : 0) |
            (s.pmOk ? L_PM_OK : 0) | (s.pmFresh ? L_PM_FRESH : 0) | (s.envOk ? L_ENV_OK : 0);
  }
  void get(Sample& s) const {
    s = Sample();
    s.seq = seq;
    s.uptimeS = up;
    s.co2Ok = flags & L_CO2_OK;
    s.co2Warm = flags & L_CO2_WARM;
    s.co2Swapped = flags & L_CO2_SWAP;
    s.co2Rx = co2Rx;
    s.co2 = unpack(co2, 1);
    s.co2Avg = co2Avg == 0xFFFF ? NAN : co2Avg / 10.0f;
    s.co2TempC = co2TempC;
    s.pmOk = flags & L_PM_OK;
    s.pmFresh = flags & L_PM_FRESH;
    s.pmState = pmState;
    s.pmPeriodS = pmPeriodS;
    s.pmNextS = pmNextS;
    s.pm25 = unpack(pm25, 10);
    s.pm10 = unpack(pm10, 10);
    s.envOk = flags & L_ENV_OK;
    s.envAddr = envAddr;
    s.tempC = unpack(tempC, 100);
    s.rh = unpack(rh, 10);
    s.pressure = unpack(pressure, 10, 900);
    s.gasKohm = gas;
    s.vocIdx = unpack(voc, 10);
    s.dewPoint = unpack(dew, 10);
    s.absHum = unpack(ah, 10);
  }
};
bool DataStore::begin() {
  static_assert(sizeof(LiveRec) <= 44, "point en direct compact");
  mutex_ = xSemaphoreCreateMutex();
  live_ = (LiveRec*)calloc(LIVE_CAPACITY, sizeof(LiveRec));
  pendCap_ = PEND_MIN;
  pend_ = (MinuteRec*)calloc(pendCap_, sizeof(MinuteRec));
  // Formate la partition au premier démarrage (quelques secondes).
  fsOk_ = LittleFS.begin(true) && loadSegments();
  lastFlushMs_ = millis();
  return mutex_ && live_ && pend_;
}

String DataStore::segPath(uint32_t num) const {
  char p[24];
  snprintf(p, sizeof(p), "%s/%08lu.bin", HIST_DIR, (unsigned long)num);
  return p;
}

// Relit la liste des fichiers d'historique au démarrage.
bool DataStore::loadSegments() {
  nSegs_ = 0;
  fCount_ = 0;
  lastSealed_ = false;
  if (!LittleFS.exists(HIST_DIR) && !LittleFS.mkdir(HIST_DIR)) return false;
  File dir = LittleFS.open(HIST_DIR);
  if (!dir || !dir.isDirectory()) return false;
  Seg found[MAX_SEGS * 2];
  bool partial[MAX_SEGS * 2] = {};
  size_t n = 0;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    const char* name = strrchr(f.name(), '/');
    name = name ? name + 1 : f.name();
    const uint32_t num = strtoul(name, nullptr, 10);
    const size_t size = f.size();
    f.close();
    if (n < MAX_SEGS * 2) {
      partial[n] = size % REC != 0;
      found[n++] = {num, (uint32_t)min<size_t>(size / REC, SEG_RECS)};
    }
  }
  dir.close();
  // tri par numéro (petite liste)
  for (size_t i = 1; i < n; i++)
    for (size_t j = i; j > 0 && found[j - 1].num > found[j].num; j--) {
      std::swap(found[j - 1], found[j]);
      std::swap(partial[j - 1], partial[j]);
    }
  // on ne garde que les HIST_FLASH_DAYS plus récents
  const size_t skip = n > HIST_FLASH_DAYS ? n - HIST_FLASH_DAYS : 0;
  for (size_t i = 0; i < skip; i++) LittleFS.remove(segPath(found[i].num));
  for (size_t i = skip; i < n; i++) {
    segs_[nSegs_++] = found[i];
    fCount_ += found[i].count;
  }
  // une écriture coupée (perte de courant) laisse un morceau d'enregistrement :
  // on n'ajoute plus rien à ce fichier
  lastSealed_ = n && partial[n - 1];
  return true;
}

bool DataStore::appendRec(const MinuteRec& r) {
  if (nSegs_ == 0 || lastSealed_ || segs_[nSegs_ - 1].count >= SEG_RECS) {
    const uint32_t num = nSegs_ ? segs_[nSegs_ - 1].num + 1 : 0;
    if (nSegs_ >= HIST_FLASH_DAYS) {
      LittleFS.remove(segPath(segs_[0].num));
      fCount_ -= segs_[0].count;
      memmove(segs_, segs_ + 1, (nSegs_ - 1) * sizeof(Seg));
      nSegs_--;
    }
    segs_[nSegs_++] = {num, 0};
    lastSealed_ = false;
  }
  Seg& last = segs_[nSegs_ - 1];
  File f = LittleFS.open(segPath(last.num), "a");
  if (!f) return false;
  const bool ok = f.write((const uint8_t*)&r, REC) == REC;
  f.close();
  if (!ok) {
    lastSealed_ = true;
    return false;
  }
  last.count++;
  fCount_++;
  return true;
}

void DataStore::add(const Sample& s) {
  if (!live_) return;
  Lock l(mutex_);
  live_[(liveHead_ + liveCount_) % LIVE_CAPACITY].set(s);
  if (liveCount_ < LIVE_CAPACITY) liveCount_++;
  else liveHead_ = (liveHead_ + 1) % LIVE_CAPACITY;

  if (minuteStart_ == 0) minuteStart_ = s.uptimeS ? s.uptimeS : 1;
  if (s.uptimeS - minuteStart_ >= 60) flushMinute(s.uptimeS);
  accumulate(s);
}

void DataStore::accumulate(const Sample& s) {
  // Le CO2 en préchauffage n'est pas fiable : on ne l'archive pas.
  if (s.co2Ok && !s.co2Warm) { sum_[F_CO2] += s.co2; n_[F_CO2]++; }
  if (s.envOk) {
    sum_[F_T] += s.tempC; n_[F_T]++;
    sum_[F_RH] += s.rh; n_[F_RH]++;
    sum_[F_P] += s.pressure; n_[F_P]++;
    if (!isnan(s.vocIdx)) { sum_[F_VOC] += s.vocIdx; n_[F_VOC]++; }
    if (!isnan(s.dewPoint)) { sum_[F_DEW] += s.dewPoint; n_[F_DEW]++; }
  }
  if (s.pmFresh) { lastPm25_ = s.pm25; lastPm10_ = s.pm10; }
}

void DataStore::flushMinute(uint32_t uptimeS) {
  MinuteRec r;
  r.up = uptimeS;
  r.ts = timekeeper::epochForUptime(uptimeS);
  r.co2 = scaled(sum_[F_CO2], n_[F_CO2], 1);
  r.tempC = scaled(sum_[F_T], n_[F_T], 100);
  r.rh = scaled(sum_[F_RH], n_[F_RH], 10);
  r.pressure = scaled(sum_[F_P], n_[F_P], 10, 900);
  r.voc = scaled(sum_[F_VOC], n_[F_VOC], 10);
  r.dew = scaled(sum_[F_DEW], n_[F_DEW], 10);
  r.pm25 = scaledF(lastPm25_, 10);
  r.pm10 = scaledF(lastPm10_, 10);

  // Une minute sans aucune mesure n'est pas archivée.
  bool any = r.co2 != INT16_MIN || r.tempC != INT16_MIN || r.pm25 != INT16_MIN;
  if (any) {
    if (pendCount_ == pendCap_ && pendCap_ < PEND_MAX) resizePend(min(pendCap_ * 2, PEND_MAX));
    pend_[(pendHead_ + pendCount_) % pendCap_] = r;
    if (pendCount_ < pendCap_) pendCount_++;
    else pendHead_ = (pendHead_ + 1) % pendCap_;  // plein : la plus ancienne est perdue
  }

  memset(sum_, 0, sizeof(sum_));
  memset(n_, 0, sizeof(n_));
  lastPm25_ = lastPm10_ = NAN;
  minuteStart_ = uptimeS;
}

void DataStore::tick() {
  if (timekeeper::takeSyncedFlag()) {
    Lock l(mutex_);
    // L'heure vient d'arriver : on date les minutes prises avant.
    for (size_t i = 0; i < pendCount_; i++) {
      MinuteRec& r = pend_[(pendHead_ + i) % pendCap_];
      if (r.ts == 0) r.ts = timekeeper::epochForUptime(r.up);
    }
  }
  if (millis() - lastFlushMs_ >= HIST_FLUSH_MS) {
    lastFlushMs_ = millis();
    Lock l(mutex_);
    flushToFlash();
  }
}

// Écrit en flash les minutes datées, dans l'ordre. Celles sans heure restent
// en RAM (perdues si on débranche avant que l'heure soit connue).
void DataStore::flushToFlash() {
  if (!fsOk_ || pendCount_ == 0) return;
  size_t keep = 0;
  bool failed = false;
  for (size_t i = 0; i < pendCount_; i++) {
    MinuteRec r = pend_[(pendHead_ + i) % pendCap_];
    if (r.ts != 0 && !failed) {
      if (appendRec(r)) continue;
      failed = true;  // flash pleine ou erreur : on garde le reste en RAM
    }
    pend_[(pendHead_ + keep++) % pendCap_] = r;
  }
  pendCount_ = keep;
  if (pendCap_ > PEND_MIN && pendCount_ <= PEND_MIN / 2) resizePend(PEND_MIN);
}

// Remet les minutes en attente dans l'ordre dans un tampon de cap places. Si
// la mémoire manque, on garde l'ancien tampon (les plus anciennes seront
// perdues quand il sera plein).
void DataStore::resizePend(size_t cap) {
  MinuteRec* p = (MinuteRec*)malloc(cap * sizeof(MinuteRec));
  if (!p) return;
  const size_t n = min(pendCount_, cap);
  for (size_t i = 0; i < n; i++) p[i] = pend_[(pendHead_ + pendCount_ - n + i) % pendCap_];
  free(pend_);
  pend_ = p;
  pendCap_ = cap;
  pendHead_ = 0;
  pendCount_ = n;
}

size_t DataStore::liveCount() {
  Lock l(mutex_);
  return liveCount_;
}

bool DataStore::liveAt(size_t i, Sample& out) {
  Lock l(mutex_);
  if (i >= liveCount_) return false;
  live_[(liveHead_ + i) % LIVE_CAPACITY].get(out);
  return true;
}

uint32_t DataStore::flashCount() {
  Lock l(mutex_);
  return fCount_;
}

size_t DataStore::flashRead(uint32_t first, MinuteRec* out, size_t n) {
  Lock l(mutex_);
  if (!fsOk_ || first >= fCount_) return 0;
  size_t got = 0, si = 0;
  uint32_t off = first;
  while (si < nSegs_ && off >= segs_[si].count) off -= segs_[si++].count;
  while (got < n && si < nSegs_) {
    const size_t run = min<size_t>(segs_[si].count - off, n - got);
    File f = LittleFS.open(segPath(segs_[si].num), "r");
    if (!f) break;
    f.seek(off * REC);
    const size_t bytes = f.read((uint8_t*)(out + got), run * REC);
    f.close();
    got += bytes / REC;
    if (bytes != run * REC) break;
    si++;
    off = 0;
  }
  return got;
}

uint32_t DataStore::flashLowerBound(uint32_t ts) {
  uint32_t lo = 0, hi = flashCount();
  while (lo < hi) {
    uint32_t mid = (lo + hi) / 2;
    MinuteRec r;
    if (flashRead(mid, &r, 1) != 1) break;
    if (r.ts < ts) lo = mid + 1;
    else hi = mid;
  }
  return lo;
}

size_t DataStore::pendingCopy(MinuteRec* out, size_t max, size_t from) {
  Lock l(mutex_);
  if (from >= pendCount_) return 0;
  const size_t n = min(max, pendCount_ - from);
  for (size_t i = 0; i < n; i++) out[i] = pend_[(pendHead_ + from + i) % pendCap_];
  return n;
}

bool DataStore::clearHistory() {
  Lock l(mutex_);
  pendHead_ = pendCount_ = 0;
  if (!fsOk_) return false;
  for (size_t i = 0; i < nSegs_; i++) LittleFS.remove(segPath(segs_[i].num));
  return loadSegments();
}

size_t DataStore::fsUsed() { return fsOk_ ? LittleFS.usedBytes() : 0; }
size_t DataStore::fsTotal() { return fsOk_ ? LittleFS.totalBytes() : 0; }
