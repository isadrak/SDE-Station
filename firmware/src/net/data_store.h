#pragma once
#include <Arduino.h>
#include <freertos/semphr.h>

#include "../app/sample.h"
#include "../config.h"

// Mémoire des mesures, partagée entre la tâche comms (qui écrit) et le
// serveur web (qui lit) :
//   - les derniers points (LIVE_CAPACITY), compactés en 44 octets, pour
//     l'affichage en direct ;
//   - les moyennes à la minute : d'abord en RAM, puis écrites en flash
//     (LittleFS) dès que l'heure réelle est connue. Elles survivent aux
//     redémarrages et aux mises à jour.
// En flash, un fichier par tranche de 1440 minutes (/h/00000012.bin...), écrit
// seulement en ajout : LittleFS réécrit tout ce qui suit quand on modifie le
// milieu d'un fichier, donc un gros fichier circulaire userait la flash et
// demanderait autant d'espace libre. Le plus ancien fichier est supprimé quand
// il y en a plus de HIST_FLASH_DAYS.
class DataStore {
 public:
  static constexpr size_t LIVE_CAPACITY = 300;   // 10 min à 2 s (13 ko de tas)
  // Minutes pas encore en flash. Avec l'heure, il y en a au plus 5 (écriture
  // toutes les 5 min) : le tampon reste petit. Sans heure, il grandit jusqu'à
  // 12 h ; c'est sans Internet, donc sans le TLS de l'accès à distance qui
  // a besoin de cette mémoire.
  static constexpr size_t PEND_MIN = 16;
  static constexpr size_t PEND_MAX = 720;
  static constexpr uint32_t FLASH_CAPACITY = HIST_FLASH_DAYS * 1440UL;
  static constexpr uint32_t SEG_RECS = 1440;
  static constexpr size_t MAX_SEGS = HIST_FLASH_DAYS + 1;

  bool begin();
  void add(const Sample& s);
  // À appeler régulièrement (tâche comms) : date les minutes en attente
  // quand l'heure arrive et écrit en flash.
  void tick();

  size_t liveCount();
  bool liveAt(size_t i, Sample& out);

  // Historique en flash : index 0 = le plus ancien.
  uint32_t flashCount();
  size_t flashRead(uint32_t first, MinuteRec* out, size_t n);
  uint32_t flashLowerBound(uint32_t ts);  // premier index avec ts >= ts
  // Minutes pas encore en flash (heure inconnue ou écriture à venir).
  // Copie au plus max minutes en attente à partir de la n° from (par lots :
  // une copie entière prendrait 34 ko de tas d'un coup).
  size_t pendingCopy(MinuteRec* out, size_t max, size_t from);

  bool clearHistory();
  bool flashOk() const { return fsOk_; }
  size_t fsUsed();
  size_t fsTotal();

 private:
  void flushMinute(uint32_t uptimeS);
  void accumulate(const Sample& s);
  void flushToFlash();
  bool loadSegments();
  String segPath(uint32_t num) const;
  bool appendRec(const MinuteRec& r);

  void resizePend(size_t cap);

  struct LiveRec;
  SemaphoreHandle_t mutex_ = nullptr;
  LiveRec* live_ = nullptr;
  size_t liveHead_ = 0, liveCount_ = 0;
  MinuteRec* pend_ = nullptr;
  size_t pendCap_ = 0, pendHead_ = 0, pendCount_ = 0;

  bool fsOk_ = false;
  struct Seg { uint32_t num, count; };
  Seg segs_[MAX_SEGS] = {};
  size_t nSegs_ = 0;
  bool lastSealed_ = false;  // dernier fichier plein ou abîmé : en ouvrir un autre
  uint32_t fCount_ = 0;
  uint32_t lastFlushMs_ = 0;

  uint32_t minuteStart_ = 0;
  double sum_[6] = {};
  uint16_t n_[6] = {};
  float lastPm25_ = NAN, lastPm10_ = NAN;
};
