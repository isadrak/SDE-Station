#include "time_keeper.h"

#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

namespace timekeeper {
namespace {
volatile Source src = Source::None;
volatile bool synced = false;
volatile uint32_t lastNtpMs = 0;

void onNtp(struct timeval*) {
  src = Source::Internet;
  lastNtpMs = millis();
  synced = true;
}
}  // namespace

void begin(const char* tz) {
  sntp_set_time_sync_notification_cb(onNtp);
  // NTP se relance tout seul ; sans Internet il échoue sans bruit.
  configTzTime(tz, "pool.ntp.org", "time.google.com", "time.nist.gov");
}

bool valid() { return time(nullptr) > 1700000000; }
Source source() { return valid() ? src : Source::None; }
const char* sourceName() {
  switch (source()) {
    case Source::Internet: return "internet";
    case Source::Browser: return "navigateur";
    default: return "aucune";
  }
}
uint32_t now() { return valid() ? (uint32_t)time(nullptr) : 0; }

uint32_t epochForUptime(uint32_t up) {
  if (!valid()) return 0;
  uint32_t upNow = millis() / 1000;
  return now() - (upNow - up);
}

bool setFromBrowser(uint32_t epoch) {
  if (epoch < 1700000000) return false;
  // L'heure d'Internet a priorité pendant 6 h après sa dernière synchro.
  if (src == Source::Internet && millis() - lastNtpMs < 6UL * 3600UL * 1000UL) return false;
  int32_t diff = (int32_t)(epoch - (uint32_t)time(nullptr));
  if (valid() && abs(diff) < 3) return false;
  struct timeval tv = {(time_t)epoch, 0};
  settimeofday(&tv, nullptr);
  src = Source::Browser;
  synced = true;
  return true;
}

bool takeSyncedFlag() {
  if (!synced) return false;
  synced = false;
  return true;
}

}  // namespace timekeeper
