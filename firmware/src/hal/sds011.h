#pragma once
#include <Arduino.h>

// Pilote UART du capteur de particules laser SDS011 (9600 8N1).
// En mode "active report" (défaut usine) il envoie une trame de 10 octets
// par seconde ; on la décode au fil de l'eau sans bloquer.
class SDS011 {
 public:
  explicit SDS011(HardwareSerial& port) : port_(port) {}

  void begin(int rxPin, int txPin);

  // À appeler souvent : consomme les octets reçus et décode les trames.
  void poll();

  // Dernière mesure reçue, si elle date de moins de maxAgeMs.
  bool latest(float& pm25, float& pm10, uint32_t maxAgeMs = 5000) const;

  // Met le laser et le ventilateur en veille (prolonge la durée de vie).
  void sleep();
  void wake();
  bool isSleeping() const { return sleeping_; }
  bool isOk() const { return lastFrameMs_ != 0 && millis() - lastFrameMs_ < 5000; }
  // Compteur de trames valides reçues (permet de détecter une nouvelle mesure)
  uint32_t frameCount() const { return frames_; }

 private:
  void sendWorkState(bool work);

  HardwareSerial& port_;
  uint8_t buf_[10];
  uint8_t idx_ = 0;
  float pm25_ = NAN, pm10_ = NAN;
  uint32_t lastFrameMs_ = 0;
  uint32_t frames_ = 0;
  bool sleeping_ = false;
};
