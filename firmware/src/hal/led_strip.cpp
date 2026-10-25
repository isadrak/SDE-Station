#include "led_strip.h"

#include <Adafruit_NeoPixel.h>  // seulement pour ColorHSV et gamma32 (fonctions statiques)
#include <Preferences.h>
#include <driver/rmt.h>

#include "../app/air_color.h"
#include "../config.h"
#include "../net/time_keeper.h"

// Envoi par le périphérique RMT, installé une seule fois. Adafruit_NeoPixel
// installait puis retirait le pilote RMT à chaque image (33 fois par seconde) :
// des allocations sans arrêt, et quand la mémoire manquait, un assert dans
// rmt_driver_install faisait planter la station (trace du 2026-10-24).
namespace {
constexpr rmt_channel_t kChannel = RMT_CHANNEL_0;
// Horloge RMT 80 MHz / 2 = 25 ns. WS2812B : 0 = 0,4 µs haut + 0,85 µs bas,
// 1 = 0,8 µs haut + 0,45 µs bas.
constexpr uint16_t kT0H = 16, kT0L = 34, kT1H = 32, kT1L = 18;
uint8_t grb[LED_COUNT * 3];
bool ready = false;
Preferences prefs;

// Octets GRB -> impulsions RMT, appelé depuis l'interruption du RMT.
void IRAM_ATTR toRmt(const void* src, rmt_item32_t* dest, size_t srcSize, size_t wanted, size_t* done,
                     size_t* items) {
  const uint8_t* p = static_cast<const uint8_t*>(src);
  size_t n = 0, i = 0;
  while (n < srcSize && i + 8 <= wanted) {
    for (int bit = 7; bit >= 0; bit--, i++) {
      const bool one = p[n] & (1 << bit);
      dest[i].level0 = 1;
      dest[i].duration0 = one ? kT1H : kT0H;
      dest[i].level1 = 0;
      dest[i].duration1 = one ? kT1L : kT0L;
    }
    n++;
  }
  *done = n;
  *items = i;
}

void show(uint32_t color, uint16_t count) {
  for (uint16_t k = 0; k < count; k++) {
    grb[k * 3] = (color >> 8) & 0xFF;
    grb[k * 3 + 1] = (color >> 16) & 0xFF;
    grb[k * 3 + 2] = color & 0xFF;
  }
  rmt_write_sample(kChannel, grb, count * 3, true);  // ~0,7 ms pour 22 LED
}
}  // namespace

void LedStrip::begin(uint8_t pin, uint16_t count) {
  count_ = count > LED_COUNT ? LED_COUNT : count;
  rmt_config_t c = RMT_DEFAULT_CONFIG_TX((gpio_num_t)pin, kChannel);
  c.clk_div = 2;
  ready = rmt_config(&c) == ESP_OK && rmt_driver_install(kChannel, 0, 0) == ESP_OK &&
          rmt_translator_init(kChannel, toRmt) == ESP_OK;
  if (ready) show(0, count_);
  prefs.begin("sde", true);
  pct_ = prefs.getUChar("led_b", LED_BRIGHTNESS_DEFAULT);
  nightOff_ = prefs.getBool("led_n", false);
  prefs.end();
  lastMs_ = millis();
}

void LedStrip::setTarget(float pos) { target_ = pos; }

void LedStrip::setBrightness(uint8_t pct) {
  if (pct > 100) pct = 100;
  pct_ = pct;
  prefs.begin("sde", false);
  prefs.putUChar("led_b", pct);
  prefs.end();
}

void LedStrip::setNightOff(bool on) {
  nightOff_ = on;
  prefs.begin("sde", false);
  prefs.putBool("led_n", on);
  prefs.end();
}

static bool isNight() {
  if (!timekeeper::valid()) return false;
  time_t t = timekeeper::now();
  struct tm lt;
  localtime_r(&t, &lt);
  return lt.tm_hour >= 22 || lt.tm_hour < 7;
}

void LedStrip::tick() {
  if (!ready) return;
  const uint32_t now = millis();
  if (now - lastShow_ < 30) return;  // ~33 images/s
  lastShow_ = now;
  const float dt = (now - lastMs_) / 1000.0f;
  lastMs_ = now;

  const float target = target_;
  // Fondu : la position rejoint la cible avec une constante de temps de
  // LED_FADE_S secondes (les mesures sont déjà moyennées).
  if (isnan(target)) cur_ = NAN;
  else if (isnan(cur_)) cur_ = target;
  else cur_ += (target - cur_) * (1 - expf(-dt / LED_FADE_S));

  uint8_t v = (uint8_t)(pct_ * 255 / 100);
  uint32_t color;
  if (pct_ == 0 || (nightOff_ && isNight())) {
    color = 0;
  } else if (isnan(cur_)) {
    // Pas encore de verdict : blanc doux qui respire
    float b = 0.15f + 0.10f * sinf(now / 1000.0f * 2 * PI / 4.0f);
    uint8_t w = (uint8_t)(v * b);
    color = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(w, w, w));
  } else {
    // Au-delà de la note 2, pulsation lente pour attirer l'attention
    float k = 1;
    if (cur_ >= 3.5f) k = 0.7f + 0.3f * sinf(now / 1000.0f * 2 * PI / 3.0f);
    uint16_t h = (uint16_t)(aircolor::hue(cur_) / 360.0f * 65535.0f);
    color = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(h, 255, (uint8_t)(v * k)));
  }

  // Limite de courant : environ 20 mA par canal à pleine valeur.
  uint32_t r = (color >> 16) & 0xFF, g = (color >> 8) & 0xFF, b = color & 0xFF;
  uint32_t ma = (r + g + b) * 20UL * count_ / 255UL;
  if (ma > LED_MAX_MA) {
    r = r * LED_MAX_MA / ma;
    g = g * LED_MAX_MA / ma;
    b = b * LED_MAX_MA / ma;
    color = Adafruit_NeoPixel::Color(r, g, b);
  }
  // Une image seulement si la couleur change (la bande la garde), et une
  // fois toutes les 5 s au cas où un parasite l'aurait brouillée.
  if (color == lastColor_ && now - lastSent_ < 5000) return;
  lastColor_ = color;
  lastSent_ = now;
  show(color, count_);
}
