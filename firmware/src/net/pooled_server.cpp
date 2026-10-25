#include "pooled_server.h"

void PooledWebServer::begin() {
  WebServer::begin();
  _server.setNoDelay(true);  // pas d'attente de Nagle sur les petites réponses JSON
}

void PooledWebServer::accept() {
  for (;;) {
    WiFiClient c = _server.available();
    if (!c) return;
    int slot = -1, oldest = 0;
    for (int i = 0; i < kSlots; i++) {
      if (!since_[i]) { slot = i; break; }  // since_ = 0 : emplacement libre
      if (since_[i] < since_[oldest]) oldest = i;
    }
    if (slot < 0) {  // réserve pleine : on ferme la connexion la plus ancienne
      slot = oldest;
      slots_[slot].stop();
    }
    slots_[slot] = c;
    since_[slot] = millis() | 1;
  }
}

void PooledWebServer::handleClient() {
  accept();
  for (int i = 0; i < kSlots; i++) {
    WiFiClient& c = slots_[i];
    if (c.available()) {
      _currentClient = c;
      c = WiFiClient();
      since_[i] = 0;
      _currentStatus = HC_WAIT_READ;
      _statusChange = millis();
      if (_parseRequest(_currentClient)) {
        _currentClient.setTimeout(HTTP_MAX_SEND_WAIT / 1000);
        _contentLength = CONTENT_LENGTH_NOT_SET;
        _handleRequest();
      }
      _currentClient = WiFiClient();  // « Connection: close » : la réponse est complète
      _currentStatus = HC_NONE;
      _currentUpload.reset();
      _currentRaw.reset();
      return;  // une requête par appel : le Wi-Fi et le DNS passent entre deux
    }
    if (since_[i] && (!c.connected() || millis() - since_[i] > kIdleMs)) {
      c.stop();
      c = WiFiClient();
      since_[i] = 0;
    }
  }
}
