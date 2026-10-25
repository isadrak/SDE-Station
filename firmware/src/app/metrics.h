#pragma once
#include <math.h>

// Grandeurs dérivées calculées à partir de T et HR.
namespace metrics {

// Point de rosée (formule de Magnus, précise à ±0,4 °C entre -45 et 60 °C)
inline float dewPoint(float tC, float rh) {
  if (isnan(tC) || isnan(rh) || rh <= 0) return NAN;
  const float a = 17.62f, b = 243.12f;
  float g = logf(rh / 100.0f) + a * tC / (b + tC);
  return b * g / (a - g);
}

// Humidité absolue en g/m3
inline float absoluteHumidity(float tC, float rh) {
  if (isnan(tC) || isnan(rh)) return NAN;
  float es = 6.112f * expf(17.67f * tC / (tC + 243.5f));
  return es * rh * 2.1674f / (273.15f + tC);
}

// Indice COV relatif 0-100 : résistance gaz rapportée à la meilleure valeur
// vue depuis le démarrage (référence "air propre"). Ce n'est pas l'IAQ
// calibré de la librairie Bosch BSEC, seulement une tendance.
// La résistance brute bouge de quelques % d'une mesure à l'autre (bruit de la
// plaque chauffante) : elle est d'abord lissée en logarithme avec une
// constante de temps d'environ 1 min, et la référence suit la valeur lissée,
// pour qu'un seul point haut ne la décale pas.
class GasIndex {
 public:
  float update(float gasKohm, float dtS) {
    if (isnan(gasKohm) || gasKohm <= 0) return NAN;
    const float lg = logf(gasKohm);
    if (isnan(smooth_)) smooth_ = lg;
    else smooth_ += (lg - smooth_) * (dtS / (kTauS + dtS));
    if (smooth_ > baseline_ || isnan(baseline_)) baseline_ = smooth_;
    else baseline_ -= (baseline_ - smooth_) * 0.00025f * dtS;  // lente dérive
    float idx = 100.0f * expf(smooth_ - baseline_);
    return idx > 100.0f ? 100.0f : idx;
  }

 private:
  static constexpr float kTauS = 60.0f;
  float smooth_ = NAN;
  float baseline_ = NAN;
};

}  // namespace metrics
