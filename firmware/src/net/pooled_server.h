#pragma once
#include <WebServer.h>

// Le WebServer d'Arduino ne s'occupe que d'une connexion à la fois. Chrome
// ouvre souvent des connexions d'avance sans rien envoyer dessus : chacune
// bloquait alors tout le serveur pendant 5 s, et les autres requêtes (page,
// /api/info…) finissaient en délai dépassé, d'où les « il faut rafraîchir ».
// Ici, les connexions sont acceptées dès qu'elles arrivent, gardées dans une
// petite réserve, et on sert la première qui a envoyé sa requête.
class PooledWebServer : public WebServer {
 public:
  explicit PooledWebServer(int port) : WebServer(port) {}
  void begin() override;
  void handleClient() override;

 private:
  // Sockets lwIP : 16 au total, et chaque connexion lue prend un tampon de
  // 1,4 ko : 4 suffisent (le plus ancien est fermé quand il en arrive un autre).
  static constexpr int kSlots = 4;
  static constexpr uint32_t kIdleMs = 8000;  // connexion muette fermée après 8 s
  void accept();
  WiFiClient slots_[kSlots];
  uint32_t since_[kSlots] = {};
};
