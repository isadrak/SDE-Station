#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#include "../app/sample.h"

// Fonctions de l'application (main.cpp) partagées entre le port USB et le
// serveur web, pour que les deux parlent exactement le même JSON.
void buildInfo(JsonDocument& doc);
void sampleToJson(const Sample& s, JsonObject out);
// Exécute une commande texte (« pm.now », « interval 5000 »...) et remplit
// la réponse {type:"ack"...} ou {type:"info"...}.
void runCommand(String line, JsonDocument& reply);
// État complet de la station (/api/info, et publié à distance).
void buildFullInfo(JsonDocument& doc);
