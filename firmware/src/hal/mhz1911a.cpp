#include "mhz1911a.h"

void MHZ1911A::begin(int pinA, int pinB) {
  pinA_ = pinA;
  pinB_ = pinB;
  openBus();
}

void MHZ1911A::openBus() {
  if (open_) bus_.end();
  // pinA = broche TX du capteur (IO16), pinB = broche RX du capteur (IO17).
  // SDA sur TX, SCL sur RX par défaut ; inversées si le capteur ne répond pas.
  const int sda = swapped_ ? pinB_ : pinA_;
  const int scl = swapped_ ? pinA_ : pinB_;
  // Résistances de tirage internes (~45 kohms) en secours : à 10 kHz elles
  // suffisent sur quelques centimètres de fil. La carte V2 prévoit 4,7 kohms.
  pinMode(sda, INPUT_PULLUP);
  pinMode(scl, INPUT_PULLUP);
  open_ = bus_.begin(sda, scl, CLOCK_HZ);
  bus_.setTimeOut(50);
}

void MHZ1911A::end() {
  if (open_) bus_.end();
  open_ = false;
}

void MHZ1911A::onFailure() {
  if (failures_ < 255) failures_++;
  if (!everRead_ && failures_ % 3 == 0) {
    swapped_ = !swapped_;
    openBus();
  }
}

bool MHZ1911A::checksumOk(const uint8_t* r, size_t n) {
  uint8_t sum = 0;
  for (size_t i = 0; i + 1 < n; i++) sum += r[i];
  const uint8_t cs = r[n - 1];
  return cs == (uint8_t)(0 - sum) || cs == (uint8_t)(0 - sum + 1);
}

bool MHZ1911A::read(uint16_t& co2ppm) {
  lastRxBytes_ = 0;
  if (!open_) {
    onFailure();
    return false;
  }
  bus_.beginTransmission(ADDR);
  bus_.write(0x01);
  if (bus_.endTransmission(true) != 0) {  // pas d'accusé : capteur absent ou broches inversées
    onFailure();
    return false;
  }
  delay(10);
  uint8_t r[5] = {0};
  const uint8_t got = bus_.requestFrom(ADDR, (uint8_t)sizeof(r));
  for (uint8_t i = 0; i < got && i < sizeof(r) && bus_.available(); i++) r[i] = bus_.read();
  lastRxBytes_ = got;
  if (got != sizeof(r) || r[0] != 0x01 || !checksumOk(r, sizeof(r))) {
    onFailure();
    return false;
  }
  const uint16_t ppm = ((uint16_t)r[1] << 8) | r[2];
  if (ppm > 10000) {
    onFailure();
    return false;
  }
  co2ppm = ppm;
  failures_ = 0;
  everRead_ = true;
  return true;
}

bool MHZ1911A::calibrate(uint16_t refPpm) {
  if (!open_) return false;
  if (refPpm < 400) refPpm = 400;
  if (refPpm > 1500) refPpm = 1500;
  uint8_t f[4] = {0x03, (uint8_t)(refPpm >> 8), (uint8_t)(refPpm & 0xFF), 0};
  f[3] = (uint8_t)(0 - (f[0] + f[1] + f[2]));
  bus_.beginTransmission(ADDR);
  bus_.write(f, sizeof(f));
  return bus_.endTransmission(true) == 0;
}
