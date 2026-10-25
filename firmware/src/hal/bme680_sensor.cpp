#include "bme680_sensor.h"
#include "../config.h"

void BME680Sensor::begin(int sda, int scl) {
  sda_ = sda;
  scl_ = scl;
  tryInit();
}

// Au repos, SDA et SCL doivent être hauts (pull-ups). Une ligne basse veut
// dire un court-circuit vers le GND ou un fil mal placé : dans ce cas on ne
// démarre pas l'I2C, car le pilote resterait bloqué de longues secondes.
bool BME680Sensor::checkLines() {
  pinMode(sda_, INPUT_PULLUP);
  pinMode(scl_, INPUT_PULLUP);
  delay(2);
  uint8_t fault = 0;
  for (int i = 0; i < 5; i++) {
    if (digitalRead(sda_) == LOW) fault |= 1;
    if (digitalRead(scl_) == LOW) fault |= 2;
    delayMicroseconds(200);
  }
  busFault_ = fault;
  return fault == 0;
}

bool BME680Sensor::tryInit() {
  lastTryMs_ = millis();
  if (!wireStarted_) {
    if (!checkLines()) {
      scanCount_ = 0;
      chipAddr_ = 0;
      return false;
    }
    Wire.begin(sda_, scl_);
    Wire.setClock(100000);
    Wire.setTimeOut(20);
    wireStarted_ = true;
  }
  const uint8_t candidates[] = {0x77, 0x76};
  for (uint8_t a : candidates) {
    if (bme_.begin(a, false)) {
      // Suréchantillonnage plus fort sur l'humidité et la pression, et filtre
      // IIR plus long (température et pression) : moins de bruit d'une mesure
      // à l'autre, pour ~80 ms de mesure au lieu de ~40 (période de 2 s).
      bme_.setTemperatureOversampling(BME680_OS_8X);
      bme_.setHumidityOversampling(BME680_OS_8X);
      bme_.setPressureOversampling(BME680_OS_16X);
      bme_.setIIRFilterSize(BME680_FILTER_SIZE_7);
      bme_.setGasHeater(320, 150);  // 320 °C pendant 150 ms
      addr_ = a;
      present_ = true;
      failures_ = 0;
      return true;
    }
  }
  present_ = false;
  scanBus();
  return false;
}

void BME680Sensor::scanBus() {
  uint8_t n = 0;
  uint8_t found[kMaxScan];
  for (uint8_t a = 0x08; a < 0x78 && n < kMaxScan; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) found[n++] = a;
  }
  memcpy(scanAddrs_, found, n);
  scanCount_ = n;

  // Registre 0xD0 = identifiant de puce chez Bosch (BME680, BME280, BMP280).
  chipId_ = 0;
  chipAddr_ = 0;
  for (uint8_t a : {0x77, 0x76}) {
    Wire.beginTransmission(a);
    Wire.write(0xD0);
    if (Wire.endTransmission(false) != 0) continue;
    if (Wire.requestFrom(a, (uint8_t)1) == 1) {
      chipAddr_ = a;
      chipId_ = Wire.read();
      break;
    }
  }
}

bool BME680Sensor::read(EnvReading& out) {
  if (!present_) {
    if (millis() - lastTryMs_ < BME_RETRY_MS || !tryInit()) return false;
  }
  if (!bme_.performReading()) {
    if (++failures_ >= 3) present_ = false;
    return false;
  }
  failures_ = 0;
  out.tempC = bme_.temperature;
  out.humidity = bme_.humidity;
  out.pressureHpa = bme_.pressure / 100.0f;
  out.gasKohm = bme_.gas_resistance / 1000.0f;
  return true;
}
