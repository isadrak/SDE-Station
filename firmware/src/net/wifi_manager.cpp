#include "wifi_manager.h"

#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>

#include "../config.h"
#include "event_log.h"

namespace {
Preferences prefs;
// Raison de la dernière déconnexion du Wi-Fi de la maison (codes ESP-IDF :
// 201 réseau introuvable, 15/202/204 mot de passe refusé...). 0 = aucune.
volatile uint8_t lastReason = 0;
}

void WifiManager::begin() {
  prefs.begin("sde", false);
  homeSsid_ = prefs.getString("ssid", "");
  homePass_ = prefs.getString("pass", "");
  apPass_ = prefs.getString("ap_pass", WIFI_AP_PASS_DEFAULT);

  WiFi.persistent(false);
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  // Le pilote relance sinon une connexion dès qu'un essai échoue (réseau de la
  // maison hors de portée, mot de passe faux) : il balaie alors les canaux
  // sans arrêt et le point d'accès SDE-XXXX décroche sans cesse. On gère les
  // nouveaux essais nous-mêmes, de plus en plus espacés (loop()).
  WiFi.setAutoReconnect(false);
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    lastReason = info.wifi_sta_disconnected.reason;
    elog::add("wifi maison : deconnecte (raison %u)", lastReason);
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) {
    elog::add("wifi maison : connecte, %s, %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) {
    elog::add("point d'acces : appareil connecte (%u)", WiFi.softAPgetStationNum());
  }, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) {
    elog::add("point d'acces : appareil parti (%u)", WiFi.softAPgetStationNum());
  }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

  // Nom unique par carte, tiré de l'adresse MAC : SDE-3F2A
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char name[16];
  snprintf(name, sizeof(name), "SDE-%02X%02X", mac[4], mac[5]);
  apSsid_ = name;
  startAp();
  if (homeSsid_.length()) connectHome();

  if (MDNS.begin(WIFI_HOSTNAME)) MDNS.addService("http", "tcp", 80);
}

void WifiManager::startAp() {
  WiFi.softAP(apSsid_.c_str(), apPass_.c_str());
}

void WifiManager::connectHome() {
  lastAttemptMs_ = millis();
  WiFi.begin(homeSsid_.c_str(), homePass_.c_str());
}

void WifiManager::loop() {
  if (!homeSsid_.length()) return;
  if (homeConnected()) {
    failures_ = 0;
    return;
  }
  // Chaque essai balaie les canaux pendant quelques secondes, ce qui gêne les
  // appareils branchés sur le point d'accès : 15 s, 30 s, 1, 2, puis 5 min.
  static const uint32_t kDelayS[] = {15, 30, 60, 120, 300};
  const uint32_t wait = kDelayS[failures_ < 4 ? failures_ : 4] * 1000UL;
  if (millis() - lastAttemptMs_ < wait) return;
  if (failures_ < 255) failures_++;
  WiFi.disconnect(false, false);
  connectHome();
}

void WifiManager::setHomeNetwork(const String& ssid, const String& pass) {
  homeSsid_ = ssid;
  homePass_ = pass;
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  failures_ = 0;
  WiFi.disconnect(false, false);
  lastReason = 0;
  connectHome();
}

void WifiManager::forgetHomeNetwork() {
  homeSsid_ = "";
  homePass_ = "";
  prefs.remove("ssid");
  prefs.remove("pass");
  WiFi.disconnect(false, true);
}

void WifiManager::setApPassword(const String& pass) {
  apPass_ = pass;
  prefs.putString("ap_pass", pass);
  startAp();
}

void WifiManager::eraseSettings() { prefs.clear(); }

String WifiManager::apIp() const { return WiFi.softAPIP().toString(); }
bool WifiManager::homeConnected() const { return WiFi.status() == WL_CONNECTED; }
String WifiManager::homeIp() const { return homeConnected() ? WiFi.localIP().toString() : String(); }
uint8_t WifiManager::homeError() const { return homeConnected() ? 0 : lastReason; }
int WifiManager::rssi() const { return homeConnected() ? WiFi.RSSI() : 0; }
uint8_t WifiManager::apClients() const { return WiFi.softAPgetStationNum(); }
