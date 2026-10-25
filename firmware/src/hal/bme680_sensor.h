#pragma once
#include <Adafruit_BME680.h>
#include <Wire.h>

struct EnvReading {
  float tempC;
  float humidity;     // %HR
  float pressureHpa;
  float gasKohm;      // résistance du capteur MOX (plus haut = air plus propre)
};

// Enveloppe du BME680 : détection automatique à 0x77 puis 0x76, et
// nouvelle tentative périodique tant qu'il n'est pas branché.
class BME680Sensor {
 public:
  void begin(int sda, int scl);
  bool read(EnvReading& out);
  bool isOk() const { return present_; }
  uint8_t address() const { return addr_; }

  // Diagnostic : adresses qui ont répondu au dernier scan du bus I2C, et
  // identifiant de puce lu à 0x76/0x77 (0x61 = BME680, 0x60 = BME280,
  // 0x58 = BMP280). 0 = rien lu.
  uint8_t scanCount() const { return scanCount_; }
  uint8_t scanAddr(uint8_t i) const { return scanAddrs_[i]; }
  uint8_t chipId() const { return chipId_; }
  uint8_t chipAddr() const { return chipAddr_; }
  // Ligne I2C collée à 0 au repos : bit 0 = SDA, bit 1 = SCL.
  uint8_t busFault() const { return busFault_; }

 private:
  bool tryInit();
  void scanBus();
  bool checkLines();
  int sda_ = -1;
  int scl_ = -1;
  bool wireStarted_ = false;
  volatile uint8_t busFault_ = 0;
  static constexpr uint8_t kMaxScan = 8;
  volatile uint8_t scanCount_ = 0;
  uint8_t scanAddrs_[kMaxScan] = {};
  volatile uint8_t chipId_ = 0;
  volatile uint8_t chipAddr_ = 0;

  Adafruit_BME680 bme_{&Wire};
  bool present_ = false;
  uint8_t addr_ = 0;
  uint32_t lastTryMs_ = 0;
  uint8_t failures_ = 0;
};
