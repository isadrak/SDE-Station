#pragma once
#include <Arduino.h>

// Bande LED adressable (WS2812B / SK6812, 5 V) tournée vers le mur.
// La couleur suit la position de qualité de l'air (0 à 4, voir app/air_color.h)
// avec un fondu lent, pour que le changement soit progressif.
class LedStrip {
 public:
  void begin(uint8_t pin, uint16_t count);
  // Position cible, NAN = pas encore de verdict (préchauffage, aucun capteur).
  void setTarget(float pos);
  // Luminosité 0 à 100 % (0 = éteinte). Gardée en mémoire.
  void setBrightness(uint8_t pct);
  uint8_t brightness() const { return pct_; }
  // Mode nuit : bande éteinte de 22 h à 7 h (heure locale, si connue). Gardé en mémoire.
  void setNightOff(bool on);
  bool nightOff() const { return nightOff_; }
  float position() const { return cur_; }
  uint16_t count() const { return count_; }
  // À appeler souvent (au moins 20 fois par seconde).
  void tick();

 private:
  uint16_t count_ = 0;
  volatile uint8_t pct_ = 0;
  volatile bool nightOff_ = false;
  volatile float target_ = NAN;
  float cur_ = NAN;
  uint32_t lastMs_ = 0;
  uint32_t lastShow_ = 0;
  uint32_t lastSent_ = 0;
  uint32_t lastColor_ = 0xFFFFFFFF;
};
