#include "sds011.h"

void SDS011::begin(int rxPin, int txPin) {
  port_.begin(9600, SERIAL_8N1, rxPin, txPin);
}

void SDS011::poll() {
  while (port_.available()) {
    uint8_t b = (uint8_t)port_.read();

    // Recherche de l'en-tête AA C0
    if (idx_ == 0 && b != 0xAA) continue;
    if (idx_ == 1 && b != 0xC0) {
      idx_ = (b == 0xAA) ? 1 : 0;
      continue;
    }
    buf_[idx_++] = b;

    if (idx_ == 10) {
      idx_ = 0;
      uint8_t sum = 0;
      for (int i = 2; i < 8; i++) sum += buf_[i];
      if (buf_[9] != 0xAB || buf_[8] != sum) continue;

      pm25_ = (buf_[2] | (buf_[3] << 8)) / 10.0f;
      pm10_ = (buf_[4] | (buf_[5] << 8)) / 10.0f;
      lastFrameMs_ = millis();
      frames_++;
    }
  }
}

bool SDS011::latest(float& pm25, float& pm10, uint32_t maxAgeMs) const {
  if (lastFrameMs_ == 0 || millis() - lastFrameMs_ > maxAgeMs) return false;
  pm25 = pm25_;
  pm10 = pm10_;
  return true;
}

void SDS011::sendWorkState(bool work) {
  uint8_t cmd[19] = {0xAA, 0xB4, 0x06, 0x01, (uint8_t)(work ? 1 : 0)};
  cmd[15] = 0xFF;  // tous les capteurs
  cmd[16] = 0xFF;
  uint8_t sum = 0;
  for (int i = 2; i < 17; i++) sum += cmd[i];
  cmd[17] = sum;
  cmd[18] = 0xAB;
  port_.write(cmd, sizeof(cmd));
  port_.flush();
}

void SDS011::sleep() {
  sendWorkState(false);
  sleeping_ = true;
}

void SDS011::wake() {
  sendWorkState(true);
  sleeping_ = false;
}
