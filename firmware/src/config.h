#pragma once

// Brochage de la carrier board (voir schéma KiCad).
// Les labels du schéma désignent la broche du CAPTEUR :
// MHZ_TX (TX du capteur) arrive sur IO16 = RX de l'ESP32, etc.

// MH-Z19B (CO2, UART2)
#define PIN_MHZ_RX 16  // ESP32 RX  <- TX du MH-Z19B
#define PIN_MHZ_TX 17  // ESP32 TX  -> RX du MH-Z19B

// SDS011 (particules, UART1)
#define PIN_SDS_RX 26  // ESP32 RX  <- TX du SDS011
#define PIN_SDS_TX 27  // ESP32 TX  -> RX du SDS011

// BME680 (I2C)
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

// Bande LED adressable WS2812B / SK6812 5 V (halo mural, couleur = qualité de l'air).
// Fil de données via une résistance de 330 ohms, idéalement à travers un
// 74AHCT1G125 (3,3 V -> 5 V) sur la carte V2.
#define PIN_LED 18
#define LED_COUNT 22                // 60 LED/m, ~37 cm en U autour du socle
#define LED_BRIGHTNESS_DEFAULT 40   // %, réglable par la commande « led <0-100> »
#define LED_MAX_MA 600              // plafond de courant de la bande
#define LED_FADE_S 8.0f             // constante de temps du fondu de couleur

// Liaison PC (USB)
#define SERIAL_BAUD 115200

// Période d'acquisition par défaut (ms), modifiable à chaud par commande
#define SAMPLE_PERIOD_MS_DEFAULT 2000
#define SAMPLE_PERIOD_MS_MIN 1000
#define SAMPLE_PERIOD_MS_MAX 60000

// Taille de la moyenne mobile (nombre d'échantillons)
#define MOVING_AVG_WINDOW 10

// Préchauffage du MH-Z19B (datasheet : 3 minutes)
#define MHZ_WARMUP_MS (3UL * 60UL * 1000UL)

// Plage du MH-Z19B, envoyée au démarrage (commande 0x99) : 2000, 5000 ou 10000 ppm.
// Certains modules sortent d'usine en 0-2000 et plafonnent alors à 2000 ppm.
#define CO2_RANGE_PPM 5000

// Watchdog des tâches (s)
#define WDT_TIMEOUT_S 15

// Nouvelle tentative de détection du BME680 (ms)
#define BME_RETRY_MS 10000

// Cycle du SDS011 : le laser a une durée de vie d'environ 8000 h en continu.
// Par défaut il dort, se réveille toutes les 5 minutes, laisse tourner le
// ventilateur 30 s pour renouveler l'air, puis moyenne 10 s de mesures.
// Avec ce cycle, il ne travaille que ~13 % du temps.
#define PM_CYCLE_S_DEFAULT 300
#define PM_CYCLE_S_MIN 60
#define PM_CYCLE_S_MAX 3600
#define PM_WARMUP_MS 30000
#define PM_MEASURE_MS 10000

// Wi-Fi : point d'accès « SDE-XXXX » toujours actif + Wi-Fi de la maison
// optionnel (configuré depuis l'appli, gardé en mémoire).
#define WIFI_AP_PASS_DEFAULT "sde-station"  // 8 caractères minimum
#define WIFI_HOSTNAME "sde"                 // http://sde.local
#define HTTP_PORT 80

// Fuseau horaire pour l'export CSV (règle POSIX). Québec / Ontario :
#define TIMEZONE "EST5EDT,M3.2.0,M11.1.0"

// Historique gardé en flash : 30 jours de moyennes à la minute (24 o chacune).
#define HIST_FLASH_DAYS 30
// Écriture en flash toutes les 5 minutes (au pire 5 min perdues si on débranche).
#define HIST_FLUSH_MS (5UL * 60UL * 1000UL)

// Accès à distance (serveur MQTT hébergé, EMQX Cloud Serverless, en TLS). Les identifiants se
// règlent depuis l'appli (Réglages > Accès à distance), gardés en NVS.
#define CLOUD_MQTT_PORT 8883
#define CLOUD_INFO_MS 60000        // état complet publié toutes les minutes
#define CLOUD_LIVE_IDLE_MS 60000   // une mesure par minute quand personne ne regarde
#define CLOUD_WATCH_MS 90000       // chaque mesure pendant 90 s après une demande de l'appli
#define CLOUD_CHUNK 2000           // taille d'un morceau d'historique (octets), un toutes les 200 ms
