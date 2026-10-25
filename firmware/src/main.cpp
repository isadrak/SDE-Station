// SDE - Station de Diagnostic Environnemental
//
// Architecture :
//   HAL         : src/hal/   (pilotes MH-Z19B, SDS011, BME680)
//   Application : src/app/   (moyenne mobile, point de rosée, indice COV)
//   (bande LED : src/hal/led_strip, couleur : src/app/air_color.h)
//   Réseau      : src/net/   (Wi-Fi, serveur web + API JSON, mémoire 24 h,
//                             accès à distance par MQTT : net/cloud)
//   Communication USB : ici (JSON ligne par ligne, commandes texte)
//
// Trois tâches FreeRTOS :
//   Cœur 1 : sensorTask -> acquisition + filtrage, pousse un Sample dans une file
//   Cœur 0 : commsTask  -> JSON vers l'USB, archive dans le DataStore
//   Cœur 0 : webTask    -> serveur HTTP (appli + API), Wi-Fi
// Toutes sont surveillées par le watchdog.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_core_dump.h>

#include "app/air_color.h"
#include "app/metrics.h"
#include "app/moving_average.h"
#include "app/pm_cycle.h"
#include "app/sample.h"
#include "app/watchdog.h"
#include "config.h"
#include "hal/bme680_sensor.h"
#include "hal/led_strip.h"
#include "hal/mhz19b.h"
#include "hal/sds011.h"
#include "net/api.h"
#include "net/cloud.h"
#include "net/data_store.h"
#include "net/event_log.h"
#include "net/time_keeper.h"
#include "net/web_server.h"
#include "net/wifi_manager.h"

#ifndef SDE_FW_VERSION
#define SDE_FW_VERSION "dev"
#endif

// ---------------------------------------------------------------- Données

enum class CmdType : uint8_t { Co2Zero, Co2Abc, PmCycle, PmNow, Interval };
struct Command {
  CmdType type;
  int32_t arg;
};

static QueueHandle_t sampleQueue;
static QueueHandle_t cmdQueue;
static volatile uint32_t samplePeriodMs = SAMPLE_PERIOD_MS_DEFAULT;

static MHZ19B co2Sensor(Serial2);
static SDS011 pmSensor(Serial1);
static PmCycle pmCycle(pmSensor);
static BME680Sensor envSensor;
static DataStore store;
static LedStrip leds;
static aircolor::PmAverage pm25Avg, pm10Avg;
static WifiManager wifi;
static SemaphoreHandle_t serialMutex;
// Plage du MH-Z19B : accusé de réception de 0x99 et relecture par 0x9B
static volatile bool co2RangeAck = false;
static volatile uint16_t co2RangeRead = 0;
// Auto-calibration (ABC) : désactivée à chaque démarrage, état relu par 0x7D
static volatile bool co2AbcOn = false;
static volatile int8_t co2AbcRead = -1;

// Cause du dernier démarrage, et les 6 précédentes, gardées en NVS : sans
// câble USB, c'est le seul moyen de savoir pourquoi la station redémarre
// (plantage, watchdog, chute de tension de l'alimentation…).
static const char* bootReason = "?";
static uint32_t bootCount = 0;
static String bootRecent;
// Tâches en cours sur chaque cœur quand le watchdog se déclenche, gardées en
// RAM RTC (survit au redémarrage) : dit qui bloquait, sans câble USB.
// Et les tâches surveillées en retard de plus de 5 s (« web+comms »…).
volatile uint32_t wdtPetMs[WDT_COUNT];
static RTC_NOINIT_ATTR char wdtTasks[48];
extern "C" void IRAM_ATTR esp_task_wdt_isr_user_handler(void) {
  static const char* const kNames[WDT_COUNT] = {"sensors", "comms", "web"};
  const uint32_t now = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
  size_t k = 0;
  auto put = [&](const char* n) {
    for (int i = 0; n && n[i] && i < 16 && k < sizeof(wdtTasks) - 2; i++) wdtTasks[k++] = n[i];
  };
  for (int cpu = 0; cpu < 2; cpu++) {
    put(pcTaskGetTaskName(xTaskGetCurrentTaskHandleForCPU(cpu)));
    wdtTasks[k++] = cpu == 0 ? '/' : ' ';
  }
  for (int t = 0; t < WDT_COUNT; t++)
    if (wdtPetMs[t] && (int32_t)(now - wdtPetMs[t]) > 5000) {
      put(kNames[t]);
      wdtTasks[k++] = '+';
    }
  wdtTasks[k ? k - 1 : 0] = 0;
}

static String bootWdt;  // wdtTasks du démarrage précédent, s'il y a lieu
static String bootCrash;  // résumé du dernier vidage mémoire (plantage), s'il y en a un

// Le dernier plantage laisse un vidage dans la partition coredump : on en garde
// la tâche, l'adresse et la pile d'appels, à décoder avec l'ELF du firmware.
static void readCrash() {
  if (esp_core_dump_image_check() != ESP_OK) return;
  esp_core_dump_summary_t* s = (esp_core_dump_summary_t*)malloc(sizeof(esp_core_dump_summary_t));
  if (!s) return;
  if (esp_core_dump_get_summary(s) == ESP_OK) {
    char buf[96];
    snprintf(buf, sizeof(buf), "%.16s pc=%08x cause=%u va=%08x elf=%.8s bt=", s->exc_task,
             (unsigned)s->exc_pc, (unsigned)s->ex_info.exc_cause, (unsigned)s->ex_info.exc_vaddr,
             (const char*)s->app_elf_sha256);
    bootCrash = buf;
    for (uint32_t i = 0; i < s->exc_bt_info.depth && i < 16; i++) {
      snprintf(buf, sizeof(buf), "%s%08x", i ? " " : "", (unsigned)s->exc_bt_info.bt[i]);
      bootCrash += buf;
    }
    if (s->exc_bt_info.corrupted) bootCrash += " (pile abimee)";
  }
  free(s);
}

// Nom de la pièce (« Chambre »…), choisi dans l'appli et gardé en NVS ;
// copie en RAM, lue par plusieurs tâches (info toutes les 10 s).
static char room[41];
static portMUX_TYPE roomMux = portMUX_INITIALIZER_UNLOCKED;
static String roomName() {
  char n[sizeof room];
  portENTER_CRITICAL(&roomMux);
  memcpy(n, room, sizeof n);
  portEXIT_CRITICAL(&roomMux);
  return n;
}
static void setRoomName(const char* n) {
  portENTER_CRITICAL(&roomMux);
  strlcpy(room, n, sizeof room);
  portEXIT_CRITICAL(&roomMux);
}

static void recordBoot() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: bootReason = "power_on"; break;
    case ESP_RST_SW: bootReason = "restart"; break;
    case ESP_RST_PANIC: bootReason = "panic"; break;
    case ESP_RST_INT_WDT: bootReason = "int_wdt"; break;
    case ESP_RST_TASK_WDT: bootReason = "task_wdt"; break;
    case ESP_RST_WDT: bootReason = "wdt"; break;
    case ESP_RST_BROWNOUT: bootReason = "brownout"; break;
    case ESP_RST_EXT: bootReason = "external"; break;
    case ESP_RST_DEEPSLEEP: bootReason = "deepsleep"; break;
    default: bootReason = "other"; break;
  }
  Preferences p;
  p.begin("sde", false);
  bootCount = p.getUInt("boots", 0) + 1;
  p.putUInt("boots", bootCount);
  String recent = String(bootReason) + "@" + String(bootCount);
  String old = p.getString("boot_log", "");
  int keep = 0, cut = old.length();
  for (int i = 0; i < (int)old.length(); i++)
    if (old[i] == ',' && ++keep == 6) { cut = i; break; }
  if (old.length()) recent += "," + old.substring(0, cut);
  p.putString("boot_log", recent);
  setRoomName(p.getString("room", "").c_str());
  p.end();
  bootRecent = recent;
  const bool valid = wdtTasks[0] && strnlen(wdtTasks, sizeof(wdtTasks)) < sizeof(wdtTasks);
  if (valid && (!strcmp(bootReason, "task_wdt") || (!strcmp(bootReason, "restart") && !strncmp(wdtTasks, "web ", 4))))
    bootWdt = wdtTasks;
  wdtTasks[0] = 0;
}

// Nombre de plantages d'affilée (panic ou watchdog) en tête du journal.
static int crashStreak() {
  int n = 0, start = 0;
  while (start < (int)bootRecent.length()) {
    int end = bootRecent.indexOf(',', start);
    if (end < 0) end = bootRecent.length();
    const String r = bootRecent.substring(start, bootRecent.indexOf('@', start));
    if (r != "panic" && r != "int_wdt" && r != "task_wdt" && r != "wdt") break;
    n++;
    start = end + 1;
  }
  return n;
}

static void setCo2Abc(bool on) {
  co2Sensor.setAutoCalibration(on);
  co2AbcOn = on;
  delay(100);
  co2AbcRead = co2Sensor.readAutoCalibration();
}

// ---------------------------------------------------------------- Acquisition

static void handleCommand(const Command& c) {
  switch (c.type) {
    case CmdType::Co2Zero: co2Sensor.calibrateZero(); break;
    case CmdType::Co2Abc: setCo2Abc(c.arg != 0); break;
    case CmdType::PmCycle: pmCycle.setPeriod(c.arg); break;
    case CmdType::PmNow: pmCycle.measureNow(); break;
    case CmdType::Interval: samplePeriodMs = c.arg; break;
  }
}

static void sensorTask(void*) {
  esp_task_wdt_add(nullptr);

  MovingAverage<MOVING_AVG_WINDOW> co2Avg;
  metrics::GasIndex gasIndex;
  uint32_t seq = 0;
  uint32_t nextSample = millis();
  bool co2Configured = false;

  for (;;) {
    wdtPet(WDT_SENSORS);

    Command cmd;
    while (xQueueReceive(cmdQueue, &cmd, 0) == pdTRUE) handleCommand(cmd);

    // Cycle veille / mesure du SDS011 (et lecture de son UART)
    pmCycle.update();

    if ((int32_t)(millis() - nextSample) < 0) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    nextSample += samplePeriodMs;
    if ((int32_t)(millis() - nextSample) > 0) nextSample = millis() + samplePeriodMs;

    Sample s = {};
    s.seq = seq++;
    s.uptimeS = millis() / 1000;

    // CO2
    uint16_t ppm;
    int8_t t;
    s.co2Ok = co2Sensor.read(ppm, t);
    if (s.co2Ok && !co2Configured) {
      // Renvoie la config une fois le capteur joignable (utile si RX/TX
      // ont dû être inversés après le démarrage).
      setCo2Abc(false);
      delay(100);
      co2RangeAck = co2Sensor.setRange(CO2_RANGE_PPM);
      delay(100);
      co2RangeRead = co2Sensor.readRange();
      co2Configured = true;
    }
    s.co2Warm = co2Sensor.isWarmingUp();
    s.co2Swapped = co2Sensor.isSwapped();
    s.co2Rx = co2Sensor.lastRxBytes();
    if (s.co2Ok) {
      s.co2 = ppm;
      s.co2TempC = t;
      // Pendant le préchauffage, les valeurs sont fausses : on ne les
      // fait pas entrer dans la moyenne.
      s.co2Avg = s.co2Warm ? NAN : co2Avg.add(ppm);
    }

    // Particules
    // Particules : une valeur n'est envoyée que lorsqu'un cycle de mesure
    // vient de se terminer (moyenne sur PM_MEASURE_MS).
    s.pmFresh = pmCycle.takeFresh(s.pm25, s.pm10);
    s.pmOk = pmCycle.isPresent();
    s.pmState = (uint8_t)pmCycle.state();
    s.pmPeriodS = pmCycle.periodS();
    s.pmNextS = pmCycle.secondsToNext();

    // Température, humidité, pression, COV
    EnvReading env;
    s.envOk = envSensor.read(env);
    s.envAddr = envSensor.address();
    if (s.envOk) {
      s.tempC = env.tempC;
      s.rh = env.humidity;
      s.pressure = env.pressureHpa;
      s.gasKohm = env.gasKohm;
      s.vocIdx = gasIndex.update(env.gasKohm, samplePeriodMs / 1000.0f);
      s.dewPoint = metrics::dewPoint(env.tempC, env.humidity);
      s.absHum = metrics::absoluteHumidity(env.tempC, env.humidity);
    }

    xQueueSend(sampleQueue, &s, 0);  // si la file est pleine, on perd ce point
  }
}

// ---------------------------------------------------------------- Communication

static float round1(float v) { return roundf(v * 10.0f) / 10.0f; }

void buildInfo(JsonDocument& doc) {
  doc["type"] = "info";
  doc["fw"] = SDE_FW_VERSION;
  doc["name"] = roomName();
  JsonObject boot = doc["boot"].to<JsonObject>();
  boot["reason"] = bootReason;
  boot["n"] = bootCount;
  boot["log"] = bootRecent;   // « cause@numéro », du plus récent au plus ancien
  if (bootWdt.length()) boot["wdt"] = bootWdt;
  if (bootCrash.length()) boot["crash"] = bootCrash;
  doc["period_ms"] = samplePeriodMs;
  doc["avg_window"] = MOVING_AVG_WINDOW;
  JsonObject time = doc["time"].to<JsonObject>();
  time["ok"] = timekeeper::valid();
  time["src"] = timekeeper::sourceName();
  if (timekeeper::valid()) time["epoch"] = (uint32_t)timekeeper::now();
  JsonObject sensors = doc["sensors"].to<JsonObject>();
  sensors["co2"] = co2Sensor.isOk();
  JsonObject range = doc["co2_range"].to<JsonObject>();
  range["set"] = CO2_RANGE_PPM;
  range["ack"] = co2RangeAck;
  if (co2RangeRead) range["read"] = co2RangeRead;
  JsonObject abc = doc["co2_abc"].to<JsonObject>();
  abc["on"] = co2AbcOn;
  if (co2AbcRead >= 0) abc["read"] = co2AbcRead == 1;
  sensors["pm"] = pmCycle.isPresent();
  doc["pm_cycle_s"] = pmCycle.periodS();
  sensors["env"] = envSensor.isOk();
  JsonObject led = doc["led"].to<JsonObject>();
  led["pin"] = PIN_LED;
  led["n"] = leds.count();
  led["b"] = leds.brightness();
  led["night"] = leds.nightOff();
  if (!isnan(leds.position())) led["pos"] = round1(leds.position());
  if (!envSensor.isOk() && envSensor.busFault()) {
    const uint8_t f = envSensor.busFault();
    doc["i2c_fault"] = f == 3 ? "SDA+SCL" : (f == 1 ? "SDA" : "SCL");
  } else if (!envSensor.isOk()) {
    JsonArray bus = doc["i2c"].to<JsonArray>();
    for (uint8_t i = 0; i < envSensor.scanCount(); i++) bus.add(envSensor.scanAddr(i));
    if (envSensor.chipAddr()) {
      doc["env_chip"] = envSensor.chipId();
      doc["env_chip_addr"] = envSensor.chipAddr();
    }
  }
}

static void printJson(JsonDocument& doc) {
  xSemaphoreTake(serialMutex, portMAX_DELAY);
  serializeJson(doc, Serial);
  Serial.println();
  xSemaphoreGive(serialMutex);
}

static void sendInfo() {
  JsonDocument doc;
  buildInfo(doc);
  JsonObject w = doc["wifi"].to<JsonObject>();
  w["ap_ssid"] = wifi.apSsid();
  w["home_ssid"] = wifi.homeSsid();
  w["home_ok"] = wifi.homeConnected();
  if (wifi.homeConnected()) w["home_ip"] = wifi.homeIp();
  printJson(doc);
}

static void setAck(JsonDocument& doc, const char* cmd, bool ok, const char* msg) {
  doc["type"] = "ack";
  doc["cmd"] = cmd;
  doc["ok"] = ok;
  doc["msg"] = msg;
}

void sampleToJson(const Sample& s, JsonObject doc) {
  doc["type"] = "data";
  doc["seq"] = s.seq;
  doc["up"] = s.uptimeS;
  const uint32_t ts = timekeeper::epochForUptime(s.uptimeS);
  if (ts) doc["ts"] = ts;

  JsonObject co2 = doc["co2"].to<JsonObject>();
  co2["ok"] = s.co2Ok;
  co2["warm"] = s.co2Warm;
  if (s.co2Swapped) co2["swap"] = true;
  if (!s.co2Ok) co2["rx"] = s.co2Rx;
  // Pendant le préchauffage (3 min), aucune valeur n'est envoyée : elles
  // sont fausses et ne doivent ni s'afficher ni entrer dans l'historique.
  if (s.co2Ok && s.co2Warm) {
    co2["warm_s"] = (MHZ_WARMUP_MS > millis()) ? (MHZ_WARMUP_MS - millis()) / 1000 : 0;
  } else if (s.co2Ok) {
    co2["ppm"] = (int)s.co2;
    if (!isnan(s.co2Avg)) co2["avg"] = round1(s.co2Avg);
    co2["t"] = (int)s.co2TempC;
  }

  JsonObject pm = doc["pm"].to<JsonObject>();
  pm["ok"] = s.pmOk;
  pm["state"] = PmCycle::stateName((PmCycle::State)s.pmState);
  pm["cycle_s"] = s.pmPeriodS;
  if (s.pmNextS) pm["next_s"] = s.pmNextS;
  if (s.pmFresh) {
    pm["pm25"] = round1(s.pm25);
    pm["pm10"] = round1(s.pm10);
  }

  JsonObject env = doc["env"].to<JsonObject>();
  env["ok"] = s.envOk;
  if (s.envOk) {
    env["addr"] = s.envAddr;
    env["t"] = roundf(s.tempC * 100) / 100;
    env["rh"] = round1(s.rh);
    env["p"] = round1(s.pressure);
    env["gas"] = round1(s.gasKohm);
    if (!isnan(s.vocIdx)) env["voc"] = round1(s.vocIdx);
    if (!isnan(s.dewPoint)) env["dew"] = round1(s.dewPoint);
    if (!isnan(s.absHum)) env["ah"] = round1(s.absHum);
  }
}

// Commandes texte, une par ligne :
//   info | interval <ms> | co2.zero | co2.abc on|off
//   pm.cycle <s> (0 = continu) | pm.now | led <0-100> | led.night on|off
//   name <nom de la pièce>
void runCommand(String line, JsonDocument& reply) {
  line.trim();
  Command c;
  if (line == "info") {
    buildInfo(reply);
    return;
  } else if (line == "co2.zero") {
    c = {CmdType::Co2Zero, 0};
    setAck(reply, "co2.zero", true, "Calibration 400 ppm envoyee (capteur a l'air exterieur depuis 20 min ?)");
  } else if (line == "co2.abc on" || line == "co2.abc off") {
    bool on = line.endsWith("on");
    c = {CmdType::Co2Abc, on ? 1 : 0};
    setAck(reply, "co2.abc", true, on ? "Auto-calibration activee" : "Auto-calibration desactivee");
  } else if (line.startsWith("pm.cycle ")) {
    long sec = line.substring(9).toInt();
    if (sec != 0 && (sec < PM_CYCLE_S_MIN || sec > PM_CYCLE_S_MAX)) {
      setAck(reply, "pm.cycle", false, "Valeur hors limites (0, ou 60 a 3600 s)");
      return;
    }
    c = {CmdType::PmCycle, (int32_t)sec};
    setAck(reply, "pm.cycle", true, sec == 0 ? "SDS011 en continu (use le laser plus vite)" : "Cycle du SDS011 modifie");
  } else if (line == "pm.now") {
    c = {CmdType::PmNow, 0};
    setAck(reply, "pm.now", true, "Mesure des particules lancee (resultat dans ~40 s)");
  } else if (line.startsWith("led ")) {
    long pct = line.substring(4).toInt();
    if (pct < 0 || pct > 100 || (pct == 0 && line.substring(4) != "0")) {
      setAck(reply, "led", false, "Valeur hors limites (0 a 100 %)");
      return;
    }
    leds.setBrightness((uint8_t)pct);
    setAck(reply, "led", true, pct == 0 ? "Bande LED eteinte" : "Luminosite de la bande LED modifiee");
    return;
  } else if (line == "led.night on" || line == "led.night off") {
    leds.setNightOff(line.endsWith("on"));
    setAck(reply, "led.night", true, leds.nightOff() ? "Bande LED eteinte de 22 h a 7 h" : "Bande LED allumee la nuit");
    return;
  } else if (line.startsWith("name ") || line == "name") {
    String n = line.length() > 5 ? line.substring(5) : String();
    n.trim();
    if (n.length() > 40) {
      setAck(reply, "name", false, "Nom trop long (40 caracteres max)");
      return;
    }
    Preferences p;
    p.begin("sde", false);
    p.putString("room", n);
    setRoomName(n.c_str());
    p.end();
    setAck(reply, "name", true, "Nom de la piece enregistre");
    return;
  } else if (line.startsWith("interval ")) {
    long ms = line.substring(9).toInt();
    if (ms < SAMPLE_PERIOD_MS_MIN || ms > SAMPLE_PERIOD_MS_MAX) {
      setAck(reply, "interval", false, "Valeur hors limites (1000 a 60000 ms)");
      return;
    }
    c = {CmdType::Interval, (int32_t)ms};
    setAck(reply, "interval", true, "Periode modifiee");
  } else if (line.startsWith("wifi.set ")) {
    // wifi.set <ssid>|<mot de passe>
    String arg = line.substring(9);
    int sep = arg.indexOf('|');
    String ssid = sep < 0 ? arg : arg.substring(0, sep);
    String pass = sep < 0 ? "" : arg.substring(sep + 1);
    if (ssid.isEmpty() || ssid.length() > 32) {
      setAck(reply, "wifi.set", false, "Nom de reseau invalide");
      return;
    }
    wifi.setHomeNetwork(ssid, pass);
    setAck(reply, "wifi.set", true, "Connexion au Wi-Fi en cours");
    return;
  } else if (line == "wifi.forget") {
    wifi.forgetHomeNetwork();
    setAck(reply, "wifi.forget", true, "Wi-Fi de la maison oublie");
    return;
  } else {
    setAck(reply, line.c_str(), false, "Commande inconnue");
    return;
  }
  xQueueSend(cmdQueue, &c, 0);
}

static void processLine(const String& line) {
  if (line.isEmpty()) return;
  JsonDocument reply;
  runCommand(line, reply);
  printJson(reply);
}

// Position de qualité de l'air (0 à 4) pour la bande LED, avec les mêmes
// mesures que la note de l'appli : particules moyennées, CO2 moyen hors
// préchauffage, humidité.
static void updateLeds(const Sample& s) {
  const uint32_t now = millis();
  if (s.pmFresh) {
    pm25Avg.add(s.pm25, now);
    pm10Avg.add(s.pm10, now);
  }
  const float co2 = (s.co2Ok && !s.co2Warm) ? s.co2Avg : NAN;
  const float rh = s.envOk ? s.rh : NAN;
  leds.setTarget(aircolor::worst(pm25Avg.value(now), pm10Avg.value(now), co2, rh));
}

static void commsTask(void*) {
  esp_task_wdt_add(nullptr);
  sendInfo();
  String line;
  uint32_t lastInfo = 0;

  for (;;) {
    wdtPet(WDT_COMMS);

    Sample s;
    if (xQueueReceive(sampleQueue, &s, pdMS_TO_TICKS(50)) == pdTRUE) {
      store.add(s);
      cloud::onSample(s);
      updateLeds(s);
      JsonDocument doc;
      sampleToJson(s, doc.to<JsonObject>());
      printJson(doc);
    }
    store.tick();  // date les minutes en attente et les écrit en flash
    elog::tick();
    leds.tick();

    while (Serial.available()) {
      char ch = (char)Serial.read();
      if (ch == '\n' || ch == '\r') {
        processLine(line);
        line = "";
      } else if (line.length() < 64) {
        line += ch;
      }
    }

    // Le serveur web n'est pas sous watchdog (voir web_server.cpp) : s'il ne
    // tourne plus depuis 2 min, on redémarre en le notant.
    if (wdtPetMs[WDT_WEB] && (int32_t)(millis() - wdtPetMs[WDT_WEB]) > 120000) {
      strlcpy(wdtTasks, "web bloque 2 min", sizeof(wdtTasks));
      elog::add("serveur web bloque depuis 2 min : redemarrage");
      esp_restart();
    }

    // Rappel périodique de l'état des capteurs (utile si l'appli se
    // connecte en cours de route).
    if (millis() - lastInfo > 10000) {
      lastInfo = millis();
      sendInfo();
    }
  }
}

// ---------------------------------------------------------------- Démarrage

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  recordBoot();
  readCrash();

  co2Sensor.begin(PIN_MHZ_RX, PIN_MHZ_TX);
  co2Sensor.setAutoCalibration(false);  // pas d'ABC en intérieur occupé en continu
  pmSensor.begin(PIN_SDS_RX, PIN_SDS_TX);
  pmCycle.begin(PM_CYCLE_S_DEFAULT);
  envSensor.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  leds.begin(PIN_LED, LED_COUNT);

  sampleQueue = xQueueCreate(10, sizeof(Sample));
  cmdQueue = xQueueCreate(8, sizeof(Command));
  // Les lignes JSON sur l'USB ne doivent jamais se mélanger.
  serialMutex = xSemaphoreCreateMutex();
  store.begin();
  elog::begin();
  elog::add("demarrage n %u : %s, firmware %s, tas %u ko", (unsigned)bootCount, bootReason, SDE_FW_VERSION,
            esp_get_free_heap_size() / 1024);
  if (bootWdt.length()) elog::add("watchdog : %s", bootWdt.c_str());
  if (bootCrash.length() && strcmp(bootReason, "power_on") && strcmp(bootReason, "restart") && strcmp(bootReason, "brownout"))
    elog::add("plantage : %s", bootCrash.c_str());
  wifi.begin();
  timekeeper::begin(TIMEZONE);

  esp_task_wdt_init(WDT_TIMEOUT_S, true);

  xTaskCreatePinnedToCore(sensorTask, "sensors", 6144, nullptr, 5, nullptr, 1);
  xTaskCreatePinnedToCore(commsTask, "comms", 6144, nullptr, 3, nullptr, 0);
  // Après 3 plantages d'affilée, l'accès à distance reste en pause pour que
  // la station reste joignable ; le réenregistrer dans l'appli le relance.
  cloud::begin(&store, &wifi, crashStreak() >= 3);  // avant le serveur web : /api/info lit son état
  webServerStart(&store, &wifi);
}

void loop() { vTaskDelete(nullptr); }
