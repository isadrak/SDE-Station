#pragma once
#include <math.h>
#include <stdint.h>

// Couleur de la bande LED, tirée des mêmes normes que la note de l'appli
// (SCALES et airVerdict() dans app/sde-analyse.html).
//
// Chaque mesure donne une position continue de 0 à 4 :
//   0 à 1 = note 5 (excellente), 1 à 2 = note 4, 2 à 3 = note 3,
//   3 à 4 = notes 2 et 1 (4 = limite haute de la note 2 et au-delà).
// À l'intérieur d'une note, la position avance en proportion de la valeur,
// ce qui donne le dégradé progressif. Comme l'appli, on retient la plus
// mauvaise mesure (règle de l'AQI de l'EPA).
namespace aircolor {

constexpr float OUTDOOR_CO2 = 420;  // ppm, comme l'appli

struct Scale {
  float best;    // valeur de la position 0
  float up[4];   // limites hautes des notes 5, 4, 3 et 2
};
constexpr Scale PM25 = {0, {9.0f, 35.4f, 55.4f, 125.4f}};   // AQI EPA 2024
constexpr Scale PM10 = {0, {54, 154, 254, 354}};            // AQI EPA
constexpr Scale CO2 = {OUTDOOR_CO2, {OUTDOOR_CO2 + 550, OUTDOOR_CO2 + 800, OUTDOOR_CO2 + 1350, 5000}};  // EN 16798-1

inline float position(const Scale& s, float v) {
  if (isnan(v)) return NAN;
  float lo = s.best;
  if (v <= lo) return 0;
  for (int i = 0; i < 4; i++) {
    if (v <= s.up[i]) return i + (v - lo) / (s.up[i] - lo);
    lo = s.up[i];
  }
  return 4;
}

// Humidité, EN 16798-1 : 30-50 / 25-60 / 20-70 %. Hors catégorie III, l'appli
// donne la note 2 (inconfort, pas toxique) : la position plafonne à 3,5.
inline float positionRh(float rh) {
  if (isnan(rh)) return NAN;
  if (rh >= 30 && rh <= 50) return fabsf(rh - 40) / 10;
  if (rh > 50) {
    if (rh <= 60) return 1 + (rh - 50) / 10;
    if (rh <= 70) return 2 + (rh - 60) / 10;
    return fminf(3 + (rh - 70) / 20, 3.5f);
  }
  if (rh >= 25) return 1 + (30 - rh) / 5;
  if (rh >= 20) return 2 + (25 - rh) / 5;
  return fminf(3 + (20 - rh) / 10, 3.5f);
}

// Pire position parmi les mesures disponibles, NAN si aucune.
inline float worst(float pm25, float pm10, float co2, float rh) {
  const float p[4] = {position(PM25, pm25), position(PM10, pm10), position(CO2, co2), positionRh(rh)};
  float w = NAN;
  for (float x : p)
    if (!isnan(x) && (isnan(w) || x > w)) w = x;
  return w;
}

// Teinte (degrés) : vert, vert-jaune, jaune, orange, rouge aux positions 0 à 4.
inline float hue(float pos) {
  static const float stops[5] = {120, 95, 50, 22, 0};
  if (isnan(pos) || pos <= 0) return stops[0];
  if (pos >= 4) return stops[4];
  int i = (int)pos;
  float f = pos - i;
  return stops[i] + (stops[i + 1] - stops[i]) * f;
}

// Moyenne glissante des particules, proche de la moyenne horaire de l'appli
// (moyenne exponentielle dans le temps, constante de 30 min). Le NowCast
// 12 h de l'appli reste la référence ; la bande réagit un peu plus vite.
class PmAverage {
 public:
  void add(float v, uint32_t nowMs) {
    if (isnan(v)) return;
    if (isnan(avg_)) avg_ = v;
    else {
      float a = 1 - expf(-(float)(nowMs - lastMs_) / TAU_MS);
      avg_ += (v - avg_) * a;
    }
    lastMs_ = nowMs;
  }
  // NAN si aucune mesure depuis 2 h (capteur débranché ou cycle très long)
  float value(uint32_t nowMs) const {
    if (isnan(avg_) || nowMs - lastMs_ > 2UL * 3600UL * 1000UL) return NAN;
    return avg_;
  }

 private:
  static constexpr float TAU_MS = 30.0f * 60.0f * 1000.0f;
  float avg_ = NAN;
  uint32_t lastMs_ = 0;
};

}  // namespace aircolor
