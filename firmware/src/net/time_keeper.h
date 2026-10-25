#pragma once
#include <Arduino.h>

// Heure réelle de la station.
// Sources, par ordre de préférence :
//   - Internet (NTP), quand la station est sur le Wi-Fi de la maison ;
//   - le navigateur qui ouvre l'appli (téléphone ou PC), sinon.
// L'heure est gardée en UTC ; le fuseau sert seulement à l'export CSV.
namespace timekeeper {

enum class Source : uint8_t { None, Browser, Internet };

void begin(const char* tz);
bool valid();
Source source();
const char* sourceName();
uint32_t now();                      // epoch UTC (s), 0 si inconnue
uint32_t epochForUptime(uint32_t up);  // 0 si l'heure est inconnue
// Réglage par le navigateur ; ignoré si Internet a donné l'heure récemment.
bool setFromBrowser(uint32_t epoch);
// Vrai une fois après chaque (re)synchronisation : la mémoire peut alors
// dater les mesures prises avant.
bool takeSyncedFlag();

}  // namespace timekeeper
