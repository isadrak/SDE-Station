#pragma once
#include <Arduino.h>
#include <functional>

#include "data_store.h"

// Lecture de l'historique (flash + minutes encore en RAM), en moyennes de
// « every » minutes, partagée par le serveur web et l'accès à distance.
// Chaque ligne est passée à row() sous forme de tableau JSON (texte et longueur,
// valable seulement pendant l'appel) :
//   [ts, up, co2, pm25, pm10, t, rh, p, voc, dew]
// (ts null = heure inconnue, utiliser up).
#define HISTORY_COLS "[\"ts\",\"up\",\"co2\",\"pm25\",\"pm10\",\"t\",\"rh\",\"p\",\"voc\",\"dew\"]"

void queryHistory(DataStore& store, uint32_t since, uint32_t until, uint32_t every,
                  const std::function<void(const char*, size_t)>& row);
