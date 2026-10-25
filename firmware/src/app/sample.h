#pragma once
#include <stdint.h>

// Un point de mesure complet, produit par la tâche capteurs.
struct Sample {
  uint32_t seq;
  uint32_t uptimeS;

  bool co2Ok, co2Warm, co2Swapped;
  uint16_t co2Rx;
  float co2, co2Avg, co2TempC;

  bool pmOk, pmFresh;
  uint8_t pmState;
  uint32_t pmPeriodS, pmNextS;
  float pm25, pm10;

  bool envOk;
  uint8_t envAddr;
  float tempC, rh, pressure, gasKohm, vocIdx, dewPoint, absHum;
};

// Moyenne d'une minute, pour l'historique (RAM puis flash).
// Valeurs entières mises à l'échelle pour tenir en 24 octets ;
// INT16_MIN = pas de donnée.
struct MinuteRec {
  uint32_t ts;        // epoch UTC de la fin de la minute (0 = heure inconnue)
  uint32_t up;        // secondes depuis le démarrage
  int16_t co2;        // ppm
  int16_t pm25;       // x10
  int16_t pm10;       // x10
  int16_t tempC;      // x100
  int16_t rh;         // x10
  int16_t pressure;   // (hPa - 900) x 10
  int16_t voc;        // x10
  int16_t dew;        // x10
};
static_assert(sizeof(MinuteRec) == 24, "format du fichier d'historique");
