#pragma once
#include <Arduino.h>

// Wi-Fi de la station :
//   - point d'accès toujours actif « SDE-XXXX » (connexion directe, adresse
//     http://192.168.4.1) ;
//   - en plus, connexion au Wi-Fi de la maison si un réseau a été enregistré
//     (adresse http://sde.local ou l'IP donnée par la box).
// Les identifiants sont gardés dans la mémoire non volatile (NVS).
class WifiManager {
 public:
  void begin();
  void loop();  // reconnexion, à appeler régulièrement

  void setHomeNetwork(const String& ssid, const String& pass);
  void forgetHomeNetwork();
  void setApPassword(const String& pass);
  void eraseSettings();  // efface tout l'espace « sde » des Preferences

  const String& apSsid() const { return apSsid_; }
  const String& apPassword() const { return apPass_; }
  String apIp() const;
  const String& homeSsid() const { return homeSsid_; }
  bool homeConnected() const;
  String homeIp() const;
  uint8_t homeError() const;  // raison ESP-IDF de la dernière déconnexion, 0 si connecté
  int rssi() const;
  uint8_t apClients() const;

 private:
  void startAp();
  void connectHome();

  String apSsid_;
  String apPass_;
  String homeSsid_;
  String homePass_;
  uint32_t lastAttemptMs_ = 0;
  uint8_t failures_ = 0;  // essais ratés d'affilée, pour espacer les suivants
};
