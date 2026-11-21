#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#include "data_store.h"

// Classement CO2 entre stations : chaque station qui participe (pseudo choisi
// dans l'appli) publie un résumé de son CO2 sur le serveur d'accès à distance,
// et lit celui des autres participants.
//
// Sujet : sde/<SDE-XXXX>/classement (retenu, toutes les 10 min). Charge :
//   {"v":1,"p":"<pseudo>","a":<moyenne 30 j>,"y":<moyenne 30 j arrêtée à hier minuit>,
//    "w":<moyenne 7 j>,"t":<moyenne d'aujourd'hui>,"d":<jours d'au moins 6 h de mesures>,
//    "s":<série de bons jours>,"xp":<points>,"ts":<epoch>,"air":[[<début epoch>,<baisse ppm>],...]}
// (moyennes en ppm, minute par minute, heure locale de la station ; « air » =
// les 3 plus grosses aérations d'aujourd'hui). Bon jour = moyenne du jour sous
// 970 ppm ; chaque bon jour vaut 100 XP, x2 après 7 jours de série, x4 après
// 14, x8 après 21 (le niveau et le badge sont calculés par l'appli). Seuls le pseudo et ces moyennes
// sortent de la station : ni le nom de la pièce, ni les autres mesures.
// Message vide = la station quitte le classement.
//
// Le serveur doit autoriser l'abonnement à sde/+/classement pour tous les
// identifiants (voir tools/atelier/LISEZMOI.md).
namespace ranking {

void begin(const String& stationId);
// Commandes : rank.name <pseudo> (2 à 20 caractères) | rank.off
bool command(const String& line, JsonDocument& reply);
bool enabled();
String pseudo();

// Depuis la tâche comms (même règle que la vague bleue de la bande LED).
void airingStart(float peak);
void airingEnd(float co2);

// Depuis la tâche cloud : résumé de cette station (calculé sur l'historique en
// flash). Faux si l'heure est inconnue ou s'il n'y a pas encore de mesures.
bool buildSelf(DataStore& store, JsonDocument& out);
// Vrai une fois après un changement de pseudo (publier tout de suite).
bool takeDirty();
// Message reçu sur sde/<id>/classement (tâche MQTT).
void onPeer(const char* topic, int topicLen, const char* data, int len);
void clearPeers();

// Classement complet pour l'appli (/api/ranking et l'accès à distance).
void toJson(JsonDocument& doc);
// État court pour /api/info.
void infoJson(JsonObject o);

}  // namespace ranking
