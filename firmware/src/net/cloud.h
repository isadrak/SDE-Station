#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#include "../app/sample.h"
#include "data_store.h"
#include "wifi_manager.h"

// Accès à distance : la station publie ses mesures sur un serveur MQTT
// hébergé (EMQX Cloud Serverless, offre gratuite), en TLS. L'appli s'y connecte en
// WebSocket sécurisé, depuis n'importe quel réseau.
//
// Sujets, sous sde/<SDE-XXXX>/ :
//   status        « online » / « offline » (retenu ; « offline » = testament)
//   info          état complet, comme /api/info (retenu, toutes les minutes)
//   live          dernière mesure, comme /api/live (retenue) : chaque mesure
//                 tant qu'une appli regarde, sinon une par minute
//   req           demandes de l'appli : {"c":"<client>","id":n,"op":...}
//                   watch                         « je regarde »
//                   history  since, until, every  comme /api/history
//                   cmd      cmd                  comme /api/cmd (sauf Wi-Fi)
//                   info                          état complet tout de suite
//   res/<client>  réponses {"id":n,...} ; l'historique arrive en morceaux
//                 {"id":n,"rows":[...]}, le dernier avec "done":true, "cols", "up", "now"
//
// Les identifiants du serveur sont gardés en NVS (espace « cloud ») et ne
// sont jamais renvoyés par l'API.
namespace cloud {

// paused : ne pas se connecter avant un nouvel enregistrement (après des plantages).
void begin(DataStore* store, WifiManager* wifi, bool paused = false);
void onSample(const Sample& s);  // depuis la tâche comms
// Enregistre le serveur (adresse vide = désactiver). Faux si l'adresse est invalide.
bool configure(const String& host, const String& user, const String& pass);
void status(JsonObject out);

}  // namespace cloud
