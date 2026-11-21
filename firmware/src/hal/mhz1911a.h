#pragma once
#include <Arduino.h>
#include <Wire.h>

// Pilote I2C du capteur CO2 NDIR Winsen MH-Z1911A (fiche V1.3).
// Le capteur choisit son mode au démarrage d'après sa broche R/T :
// haute = UART (protocole non publié), basse = I2C. La carte V2 la met à la masse.
// Adresse 0x31, horloge 10 kHz conseillée, octets de poids fort en premier.
//   Lecture  : écrire 0x01, puis lire 0x01 DATA0 DATA1 DATA2 CS
//              (CO2 = DATA0 * 256 + DATA1 ; exemple de la fiche :
//              01 02 58 01 A4 = 600 ppm)
//   Calibration : écrire 0x03 DH DL (valeur de référence 400 à 1500 ppm)
// La somme de contrôle annoncée est « -(somme) + 1 », mais l'exemple de la
// fiche correspond à « -(somme) » : les deux sont acceptées.
//
// Le capteur se branche sur le même connecteur que le MH-Z19B (IO16/IO17),
// sur un second bus I2C (Wire1) pour ne pas ralentir celui du BME680.
// Comme pour l'UART, SDA et SCL sont inversées automatiquement si besoin.
class MHZ1911A {
 public:
  static constexpr uint8_t ADDR = 0x31;
  static constexpr uint32_t CLOCK_HZ = 10000;

  explicit MHZ1911A(TwoWire& bus) : bus_(bus) {}

  // Ouvre le bus sur les deux broches du connecteur CO2.
  void begin(int pinA, int pinB);
  void end();
  // Lit la concentration. Retourne true si la réponse est valide.
  bool read(uint16_t& co2ppm);
  // Calibration sur une référence connue (400 ppm = air extérieur).
  bool calibrate(uint16_t refPpm = 400);

  bool isOk() const { return failures_ < 3 && everRead_; }
  bool everRead() const { return everRead_; }
  bool isSwapped() const { return swapped_; }
  uint8_t failures() const { return failures_; }
  // Octets reçus lors de la dernière lecture (0 = aucune réponse)
  uint16_t lastRxBytes() const { return lastRxBytes_; }

 private:
  void openBus();
  void onFailure();
  static bool checksumOk(const uint8_t* r, size_t n);

  TwoWire& bus_;
  int pinA_ = -1, pinB_ = -1;
  bool open_ = false;
  bool swapped_ = false;
  bool everRead_ = false;
  uint8_t failures_ = 0;
  uint16_t lastRxBytes_ = 0;
};
