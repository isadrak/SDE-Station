#pragma once
#include <Arduino.h>

// Progression d'une mise à jour du firmware (0 à 1), NAN hors mise à jour.
// Écrite par le serveur web, affichée en barre par la bande.
namespace ledfx {
extern volatile float otaProgress;
}

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
  // Protection après des chutes de tension : bande forcée éteinte, levée par setBrightness.
  void setGuard(bool on) { guard_ = on; }
  bool guarded() const { return guard_; }
  float position() const { return cur_; }
  uint16_t count() const { return count_; }
  // Nombre de LED de la bande (1 à LED_COUNT_MAX). Gardé en mémoire.
  void setCount(uint16_t n);
  // Animations : vague d'air frais (aération), clignotement « trouver la station »,
  // point qui glisse pendant une mesure des poussières.
  void fxWave() { waveStart_ = millis(); }
  void fxFind(uint32_t ms) { findUntil_ = millis() + ms; }
  void setPmActive(bool on) { pmActive_ = on; }
  // À appeler souvent (au moins 20 fois par seconde).
  void tick();

 private:
  uint16_t count_ = 0;
  volatile uint8_t pct_ = 0;
  volatile bool nightOff_ = false;
  volatile bool guard_ = false;
  volatile float target_ = NAN;
  float cur_ = NAN;
  volatile uint16_t wantCount_ = 0;
  uint32_t lastMs_ = 0;
  uint32_t startMs_ = 0;
  uint32_t lastShow_ = 0;
  uint32_t lastSent_ = 0;
  uint32_t lastColor_ = 0xFFFFFFFF;
  int8_t level_ = -1;          // note affichée (0 à 3), -1 = aucune
  uint32_t sweepStart_ = 0;    // changement de note : la nouvelle couleur se propage
  uint32_t sweepOld_ = 0;
  volatile uint32_t waveStart_ = 0;
  volatile uint32_t findUntil_ = 0;
  volatile bool pmActive_ = false;
  bool fxShown_ = false;       // la dernière image était une animation
  uint32_t baseColor(uint32_t now, uint8_t v) const;
};
