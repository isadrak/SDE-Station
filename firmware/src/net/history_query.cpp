#include "history_query.h"

#include <esp_task_wdt.h>

namespace {

// Moyenne de plusieurs minutes (paramètre every), en ignorant les trous.
struct Bucket {
  uint32_t ts = 0, up = 0;
  double sum[8] = {};
  uint16_t n[8] = {};
  bool empty = true;
  void add(const MinuteRec& r) {
    const int16_t v[8] = {r.co2, r.pm25, r.pm10, r.tempC, r.rh, r.pressure, r.voc, r.dew};
    for (int i = 0; i < 8; i++)
      if (v[i] != INT16_MIN) { sum[i] += v[i]; n[i]++; }
    ts = r.ts;
    up = r.up;
    empty = false;
  }
  void reset() { *this = Bucket(); }
};

// Ligne [ts, up, co2, pm25, pm10, t, rh, p, voc, dew] écrite dans un tampon
// fixe : pas de String par valeur (des dizaines de milliers de petites
// allocations pour 30 jours, qui hachaient le tas).
struct Col { float div, offset; uint8_t dec; };
constexpr Col kCols[8] = {{1, 0, 0}, {10, 0, 1}, {10, 0, 1}, {100, 0, 2}, {10, 0, 1}, {10, 900, 1}, {10, 0, 1}, {10, 0, 1}};

size_t bucketRow(const Bucket& b, char* out, size_t cap) {
  int n = b.ts ? snprintf(out, cap, "[%lu,%lu", (unsigned long)b.ts, (unsigned long)b.up)
               : snprintf(out, cap, "[null,%lu", (unsigned long)b.up);
  for (int i = 0; i < 8 && n < (int)cap; i++) {
    const Col& c = kCols[i];
    n += b.n[i] ? snprintf(out + n, cap - n, ",%.*f", c.dec, b.sum[i] / b.n[i] / c.div + c.offset)
                : snprintf(out + n, cap - n, ",null");
  }
  if (n < (int)cap - 1) out[n++] = ']';
  out[n < (int)cap ? n : cap - 1] = 0;
  return n < (int)cap ? n : cap - 1;
}

}  // namespace

void queryHistory(DataStore& store, uint32_t since, uint32_t until, uint32_t every,
                  const std::function<void(const char*, size_t)>& row) {
  Bucket b;
  uint32_t bucketKey = UINT32_MAX;
  char line[128];
  auto emit = [&]() {
    if (b.empty) return;
    row(line, bucketRow(b, line, sizeof line));
    b.reset();
  };
  auto push = [&](const MinuteRec& r) {
    uint32_t key = r.ts ? r.ts / (every * 60) : (r.up / (every * 60)) | 0x80000000u;
    if (key != bucketKey) { emit(); bucketKey = key; }
    b.add(r);
  };

  // Tampon propre à chaque appel : le serveur web et l'accès à distance
  // peuvent lire en même temps.
  constexpr size_t kBuf = 64;
  MinuteRec* buf = (MinuteRec*)malloc(kBuf * sizeof(MinuteRec));
  if (buf) {
    const uint32_t total = store.flashCount();
    for (uint32_t i = since ? store.flashLowerBound(since) : 0; i < total;) {
      size_t got = store.flashRead(i, buf, kBuf);
      if (!got) break;
      bool stop = false;
      for (size_t k = 0; k < got; k++) {
        if (buf[k].ts > until) { stop = true; break; }
        push(buf[k]);
      }
      i += got;
      esp_task_wdt_reset();
      if (stop) break;
    }
  }
  // Minutes encore en RAM (récentes, ou sans heure), par lots dans buf.
  if (buf) {
    size_t from = 0, n;
    while ((n = store.pendingCopy(buf, kBuf, from)) > 0) {
      for (size_t k = 0; k < n; k++) {
        const MinuteRec& r = buf[k];
        if (r.ts && (r.ts < since || r.ts > until)) continue;
        push(r);
      }
      from += n;
    }
    free(buf);
  }
  emit();
}
