#include "event_log.h"

#include <LittleFS.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

#include "time_keeper.h"

namespace elog {
namespace {

constexpr uint32_t kMagic = 0x53444531;  // « SDE1 »
constexpr size_t kRing = 2048;
constexpr size_t kMaxFile = 32768;
const char* const kCur = "/log.txt";
const char* const kOld = "/log.old";

// head et flushed comptent les octets écrits depuis toujours : la position
// dans buf est leur reste par kRing.
struct Ring {
  uint32_t magic, head, flushed;
  char buf[kRing];
};
RTC_NOINIT_ATTR Ring ring;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
SemaphoreHandle_t fileLock;
volatile bool ready = false;
vprintf_like_t prevVprintf;
uint32_t lastFlush = 0, lastMem = 0;

void put(const char* s, size_t n) {
  portENTER_CRITICAL(&mux);
  for (size_t i = 0; i < n; i++) ring.buf[ring.head++ % kRing] = s[i];
  if (ring.head - ring.flushed > kRing) ring.flushed = ring.head - kRing;  // lignes perdues
  portEXIT_CRITICAL(&mux);
}

// Erreurs et avertissements d'ESP-IDF (Wi-Fi, MQTT, TLS…) : dans le journal
// seulement, pas sur l'USB où ils couperaient les lignes JSON de l'appli.
// Une ligne répétée (même texte) n'est gardée qu'une fois.
// Petit tampon : ce crochet tourne dans des tâches du système à pile courte.
int hook(const char* fmt, va_list ap) {
  if (!ready || (fmt[0] != 'E' && fmt[0] != 'W')) return prevVprintf(fmt, ap);
  static uint32_t lastHash;
  char b[112];
  int n = snprintf(b, sizeof b, "+%lus ", (unsigned long)(millis() / 1000));
  const int start = n;
  n += vsnprintf(b + n, sizeof b - n, fmt, ap);
  if (n >= (int)sizeof b) n = sizeof b - 1;
  while (n > start && (b[n - 1] == '\n' || b[n - 1] == '\r')) n--;
  // « E (12345) tag: texte » : le hachage saute l'horodatage d'ESP-IDF.
  const char* body = (const char*)memchr(b + start, ')', n - start);
  body = body ? body + 1 : b + start;
  uint32_t h = 2166136261u;
  for (const char* c = body; c < b + n; c++) h = (h ^ (uint8_t)*c) * 16777619u;
  if (h == lastHash) return 0;
  lastHash = h;
  b[n++] = '\n';
  put(b, n);
  return n;
}

void flush(bool wait) {
  if (!xSemaphoreTake(fileLock, wait ? portMAX_DELAY : 0)) return;  // lecture en cours
  File f;
  char tmp[256];
  for (;;) {
    size_t n = 0;
    portENTER_CRITICAL(&mux);
    const uint32_t from = ring.flushed;
    while (n < sizeof tmp && from + n != ring.head) tmp[n] = ring.buf[(from + n) % kRing], n++;
    portEXIT_CRITICAL(&mux);
    if (!n) break;
    if (!f && !(f = LittleFS.open(kCur, "a"))) break;
    f.write((const uint8_t*)tmp, n);
    portENTER_CRITICAL(&mux);
    if ((int32_t)(from + n - ring.flushed) > 0) ring.flushed = from + n;
    portEXIT_CRITICAL(&mux);
  }
  const bool full = f && f.size() > kMaxFile;
  if (f) f.close();
  if (full) {
    LittleFS.remove(kOld);
    LittleFS.rename(kCur, kOld);
  }
  xSemaphoreGive(fileLock);
}

}  // namespace

void begin() {
  fileLock = xSemaphoreCreateMutex();
  // Mise sous tension : la RAM RTC contient n'importe quoi.
  if (ring.magic != kMagic || ring.head - ring.flushed > kRing) {
    ring.magic = kMagic;
    ring.head = ring.flushed = 0;
  }
  ready = true;
  flush(true);  // les dernières lignes avant le redémarrage
  // Le crochet peut être appelé dès son installation : prevVprintf doit
  // déjà pointer sur la sortie d'origine (vprintf, celle d'ESP-IDF par défaut).
  prevVprintf = vprintf;
  esp_log_set_vprintf(hook);
}

void add(const char* fmt, ...) {
  if (!ready) return;
  char b[176];
  int n;
  time_t t = timekeeper::now();
  if (t) {
    struct tm tm;
    localtime_r(&t, &tm);
    n = strftime(b, sizeof b, "%d/%m %H:%M:%S ", &tm);
  } else {
    n = snprintf(b, sizeof b, "+%lus ", (unsigned long)(millis() / 1000));
  }
  va_list ap;
  va_start(ap, fmt);
  n += vsnprintf(b + n, sizeof b - n, fmt, ap);
  va_end(ap);
  if (n >= (int)sizeof b) n = sizeof b - 1;
  b[n++] = '\n';
  put(b, n);
}

void tick() {
  const uint32_t now = millis();
  if (now - lastMem > 600000 || !lastMem) {
    lastMem = now;
    // Et la pile jamais utilisée de chaque tâche (octets) : près de 0 = débordement proche.
    char st[96];
    int k = 0;
    for (const char* t : {"sensors", "comms", "web", "cloud", "mqtt_task"}) {
      TaskHandle_t h = xTaskGetHandle(t);
      if (h && k < (int)sizeof st) k += snprintf(st + k, sizeof st - k, " %s %u", t, (unsigned)uxTaskGetStackHighWaterMark(h));
    }
    st[k < (int)sizeof st ? k : sizeof st - 1] = 0;
    add("memoire : tas %u ko, minimum %u ko, plus grand bloc %u ko ; pile libre%s", esp_get_free_heap_size() / 1024,
        esp_get_minimum_free_heap_size() / 1024, heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024, st);
  }
  if (now - lastFlush > 30000 || ring.head - ring.flushed > kRing / 2) {
    lastFlush = now;
    flush(false);
  }
}

void read(void (*out)(const char*, size_t, void*), void* ctx) {
  flush(true);
  xSemaphoreTake(fileLock, portMAX_DELAY);
  char tmp[512];
  for (const char* p : {kOld, kCur}) {
    File f = LittleFS.open(p, "r");
    if (!f) continue;
    size_t n;
    while ((n = f.read((uint8_t*)tmp, sizeof tmp)) > 0) out(tmp, n, ctx);
    f.close();
  }
  xSemaphoreGive(fileLock);
}

String tail(size_t max) {
  flush(true);
  xSemaphoreTake(fileLock, portMAX_DELAY);
  String s;
  File f = LittleFS.open(kCur, "r");
  if (f) {
    const size_t size = f.size();
    if (size > max) f.seek(size - max);
    s.reserve(max);
    char tmp[256];
    size_t n;
    while ((n = f.read((uint8_t*)tmp, sizeof tmp)) > 0) s.concat(tmp, n);
    f.close();
    const int nl = s.indexOf('\n');  // on commence sur une ligne entière
    if (size > max && nl >= 0) s.remove(0, nl + 1);
  }
  xSemaphoreGive(fileLock);
  return s;
}

}  // namespace elog
