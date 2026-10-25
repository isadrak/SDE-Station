#include "mhz19b.h"

void MHZ19B::begin(int rxPin, int txPin) {
  rxPin_ = rxPin;
  txPin_ = txPin;
  openPort();
}

void MHZ19B::openPort() {
  port_.end();
  if (swapped_) port_.begin(9600, SERIAL_8N1, txPin_, rxPin_);
  else port_.begin(9600, SERIAL_8N1, rxPin_, txPin_);
  port_.setTimeout(300);
}

void MHZ19B::onFailure() {
  if (failures_ < 255) failures_++;
  // Diagnostic de câblage : tant que le capteur n'a jamais répondu, on
  // essaie d'inverser RX et TX toutes les 3 tentatives ratées.
  if (!everRead_ && failures_ % 3 == 0) {
    swapped_ = !swapped_;
    openPort();
  }
}

uint8_t MHZ19B::checksum(const uint8_t* frame) {
  uint8_t sum = 0;
  for (int i = 1; i < 8; i++) sum += frame[i];
  return (uint8_t)(0xFF - sum + 1);
}

void MHZ19B::sendCommand(uint8_t cmd, uint8_t b3, uint8_t b4, uint8_t b5,
                         uint8_t b6, uint8_t b7) {
  uint8_t frame[9] = {0xFF, 0x01, cmd, b3, b4, b5, b6, b7, 0x00};
  frame[8] = checksum(frame);
  while (port_.available()) port_.read();  // purge les octets parasites
  port_.write(frame, sizeof(frame));
  port_.flush();
}

bool MHZ19B::readFrame(uint8_t cmd, uint8_t* resp, uint16_t& rxBytes) {
  rxBytes = 0;
  // Resynchronisation sur l'octet de début 0xFF
  uint32_t deadline = millis() + 300;
  int b = -1;
  while (millis() < deadline) {
    if (port_.available()) {
      b = port_.read();
      rxBytes++;
      if (b == 0xFF) break;
    } else {
      delay(2);
    }
  }
  if (b != 0xFF) return false;
  resp[0] = 0xFF;
  size_t got = port_.readBytes(resp + 1, 8);
  rxBytes += got;
  return got == 8 && resp[1] == cmd && resp[8] == checksum(resp);
}

bool MHZ19B::read(uint16_t& co2ppm, int8_t& tempC) {
  sendCommand(0x86);
  uint8_t resp[9];
  if (!readFrame(0x86, resp, lastRxBytes_)) {
    onFailure();
    return false;
  }

  co2ppm = ((uint16_t)resp[2] << 8) | resp[3];
  tempC = (int8_t)(resp[4] - 40);  // température interne, indicative
  failures_ = 0;
  everRead_ = true;
  return true;
}

void MHZ19B::calibrateZero() { sendCommand(0x87); }

void MHZ19B::setAutoCalibration(bool enabled) {
  sendCommand(0x79, enabled ? 0xA0 : 0x00);
}

bool MHZ19B::setRange(uint16_t range) {
  sendCommand(0x99, 0, 0, 0, (uint8_t)(range >> 8), (uint8_t)(range & 0xFF));
  uint8_t resp[9];
  uint16_t rx;
  return readFrame(0x99, resp, rx);
}

int8_t MHZ19B::readAutoCalibration() {
  sendCommand(0x7D);
  uint8_t resp[9];
  uint16_t rx;
  if (!readFrame(0x7D, resp, rx)) return -1;
  return resp[7] ? 1 : 0;
}

uint16_t MHZ19B::readRange() {
  sendCommand(0x9B);
  uint8_t resp[9];
  uint16_t rx;
  if (!readFrame(0x9B, resp, rx)) return 0;
  // Réponse : FF 9B 00 00 RH RL .. .. CS  (certains firmwares mettent la
  // plage en octets 4-5, d'autres en 6-7)
  uint16_t r = ((uint16_t)resp[4] << 8) | resp[5];
  if (r == 0) r = ((uint16_t)resp[6] << 8) | resp[7];
  return (r >= 1000 && r <= 10000) ? r : 0;
}
