#pragma once
#include <Arduino.h>

// Pilote UART du capteur CO2 NDIR MH-Z19B (9600 8N1, trames de 9 octets).
class MHZ19B {
 public:
  explicit MHZ19B(HardwareSerial& port) : port_(port) {}

  void begin(int rxPin, int txPin);

  // Lit la concentration. Retourne true si la trame est valide.
  bool read(uint16_t& co2ppm, int8_t& tempC);

  // Calibration du zéro : le capteur doit être à l'air extérieur (~400 ppm)
  // depuis au moins 20 minutes.
  void calibrateZero();
  void setAutoCalibration(bool enabled);
  // Plage de mesure (2000, 5000 ou 10000 ppm). La commande 0x99 est gardée
  // par le capteur en mémoire non volatile. Retourne true si le capteur a
  // accusé réception.
  bool setRange(uint16_t range);
  // Relit la plage (commande 0x9B). Retourne 0 si le capteur ne la gère pas
  // (vieux firmware du MH-Z19B).
  uint16_t readRange();
  // Relit l'état de l'auto-calibration (commande 0x7D) : 1 = active,
  // 0 = désactivée, -1 = le capteur ne gère pas cette lecture.
  int8_t readAutoCalibration();

  bool isOk() const { return failures_ < 3 && everRead_; }
  // RX et TX inversés par rapport au brochage prévu (détecté automatiquement)
  bool isSwapped() const { return swapped_; }
  // Octets reçus lors de la dernière lecture (0 = aucune réponse du tout)
  uint16_t lastRxBytes() const { return lastRxBytes_; }
  bool isWarmingUp() const { return millis() < MHZ_WARMUP_END_MS; }

 private:
  static constexpr uint32_t MHZ_WARMUP_END_MS = 3UL * 60UL * 1000UL;
  void sendCommand(uint8_t cmd, uint8_t b3 = 0, uint8_t b4 = 0, uint8_t b5 = 0,
                   uint8_t b6 = 0, uint8_t b7 = 0);
  static uint8_t checksum(const uint8_t* frame);
  // Attend une réponse de 9 octets à la commande cmd ; retourne le nombre
  // d'octets reçus dans rxBytes.
  bool readFrame(uint8_t cmd, uint8_t* resp, uint16_t& rxBytes);
  void openPort();
  void onFailure();

  HardwareSerial& port_;
  int rxPin_ = -1, txPin_ = -1;
  uint8_t failures_ = 0;
  uint16_t lastRxBytes_ = 0;
  bool everRead_ = false;
  bool swapped_ = false;
};
