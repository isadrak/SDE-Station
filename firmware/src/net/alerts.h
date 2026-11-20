#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#include "../app/sample.h"

// Alertes sur le téléphone, envoyées par la station elle-même (l'appli n'a
// pas besoin d'être ouverte) par ntfy (https://ntfy.sh) : le téléphone
// installe l'appli ntfy et s'abonne au sujet de la station (QR code de l'appli).
//   - CO2 moyen au-dessus du seuil (1200 ppm) pendant 10 min ;
//   - humidité au-dessus du seuil (60 %) pendant 3 h (risque de moisissure).
// Une alerte ne revient pas avant que la valeur soit redescendue, et au plus
// une fois toutes les 3 h (CO2) ou 12 h (humidité).
// Réglages gardés en NVS (espace « alert ») ; sujet vide = alertes coupées.
namespace alerts {

void begin(const String& stationId);
// À appeler pour chaque point de mesure (tâche comms).
void onSample(const Sample& s, const String& room);

// Commandes : alert.topic <sujet> | alert.off | alert.co2 <ppm> | alert.rh <%> | alert.test
// Retourne false si la ligne n'est pas une commande d'alerte.
bool command(const String& line, JsonDocument& reply);
void toJson(JsonObject o);

}  // namespace alerts
