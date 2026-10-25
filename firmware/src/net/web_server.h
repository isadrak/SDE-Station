#pragma once
#include "data_store.h"
#include "wifi_manager.h"

// Serveur HTTP : sert l'appli (fichiers de app/, en flash) et l'API JSON.
//   GET  /, /manifest.webmanifest, /sw.js, /icon-*.png   appli installable
//   GET  /api/info              état des capteurs, du Wi-Fi, de l'heure, de l'historique
//   GET  /api/live?since=N      points bruts dont seq > N (10 dernières minutes)
//   GET  /api/history?since=&until=&every=   moyennes (flash, 30 jours)
//   GET  /api/history.csv       export complet
//   POST /api/history/reset     pass=..
//   POST /api/reset             mode=restart, ou mode=factory&pass=.. (réglages d'usine)
//   POST /api/time              epoch=..  (heure du navigateur)
//   POST /api/update?pass=..    firmware « -ota.bin » en multipart
//   GET  /api/cloud             accès à distance (serveur MQTT) : état
//   POST /api/cloud             host=..&user=..&pass=..  (host vide = désactiver)
//   POST /api/cmd               cmd=<commande texte>
//   GET  /api/wifi/scan         réseaux visibles
//   POST /api/wifi              ssid=..&pass=..  (Wi-Fi de la maison)
//   POST /api/wifi/forget
//   POST /api/wifi/ap           old=..&pass=..  (mot de passe du point d'accès)
//   GET  /api/log               journal de la station (texte)
void webServerStart(DataStore* store, WifiManager* wifi);
