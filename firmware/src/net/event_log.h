#pragma once
#include <Arduino.h>

// Journal de la station : démarrages, Wi-Fi, accès à distance, erreurs
// d'ESP-IDF, mémoire. Sans câble USB, c'est ce qui dit ce qui s'est passé.
// Les lignes vont d'abord dans un tampon en RAM RTC, qui survit à un
// plantage : celles d'avant un plantage sont écrites en flash au démarrage
// suivant. Le reste est écrit en flash toutes les 30 s (/log.txt, puis
// /log.old quand il dépasse 32 ko).
namespace elog {

void begin();  // après le montage de LittleFS
void add(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void tick();   // tâche comms : écriture en flash, mémoire toutes les 10 min
// Tout le journal, du plus ancien au plus récent, morceau par morceau.
void read(void (*out)(const char* s, size_t n, void* ctx), void* ctx);
String tail(size_t max);  // la fin du journal, pour l'accès à distance

}  // namespace elog
