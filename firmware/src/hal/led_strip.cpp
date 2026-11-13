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
uint8_t grb[LED_COUNT_MAX * 3];
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

void send(uint16_t count) { rmt_write_sample(kChannel, grb, count * 3, true); }  // ~0,7 ms pour 22 LED

void show(uint32_t color, uint16_t count) {
  for (uint16_t k = 0; k < count; k++) {
    grb[k * 3] = (color >> 8) & 0xFF;
    grb[k * 3 + 1] = (color >> 16) & 0xFF;
    grb[k * 3 + 2] = color & 0xFF;
  }
  send(count);
}

uint32_t px[LED_COUNT_MAX];

uint32_t rgb(uint32_t r, uint32_t g, uint32_t b) { return (r << 16) | (g << 8) | b; }
// Mélange de a vers b, t de 0 à 1.
uint32_t mix(uint32_t a, uint32_t b, float t) {
  if (t <= 0) return a;
  if (t >= 1) return b;
  uint32_t out = 0;
  for (int sh = 0; sh <= 16; sh += 8) {
    float x = ((a >> sh) & 0xFF) * (1 - t) + ((b >> sh) & 0xFF) * t;
    out |= (uint32_t)x << sh;
  }
  return out;
}
uint32_t scale(uint32_t c, float k) { return mix(0, c, k); }
// Bosse douce centrée sur c, de demi-largeur w (en LED).
float bump(float i, float c, float w) {
  float d = fabsf(i - c) / w;
  return d >= 1 ? 0 : 0.5f + 0.5f * cosf(d * PI);
}
}  // namespace

void LedStrip::begin(uint8_t pin, uint16_t count) {
  rmt_config_t c = RMT_DEFAULT_CONFIG_TX((gpio_num_t)pin, kChannel);
  c.clk_div = 2;
  // Les 8 blocs mémoire du RMT (512 impulsions = 21 LED d'avance) : l'image
  // part presque entière sans recharge par interruption, que le Wi-Fi peut
  // retarder (bits faussés, bande brièvement bleue pendant les animations).
  c.mem_block_num = 8;
  ready = rmt_config(&c) == ESP_OK && rmt_driver_install(kChannel, 0, 0) == ESP_OK &&
          rmt_translator_init(kChannel, toRmt) == ESP_OK;
  prefs.begin("sde", true);
  count = prefs.getUChar("led_c", count);
  pct_ = prefs.getUChar("led_b", LED_BRIGHTNESS_DEFAULT);
  nightOff_ = prefs.getBool("led_n", false);
  prefs.end();
  count_ = count < 1 ? 1 : (count > LED_COUNT_MAX ? LED_COUNT_MAX : count);
  wantCount_ = count_;
  if (ready) show(0, LED_COUNT_MAX);  // éteint aussi les LED au-delà du nombre réglé
  lastMs_ = millis();
  startMs_ = lastMs_;
}

void LedStrip::setCount(uint16_t n) {
  if (n < 1) n = 1;
  if (n > LED_COUNT_MAX) n = LED_COUNT_MAX;
  wantCount_ = n;
  prefs.begin("sde", false);
  prefs.putUChar("led_c", (uint8_t)n);
  prefs.end();
}

void LedStrip::setTarget(float pos) { target_ = pos; }

void LedStrip::setBrightness(uint8_t pct) {
  if (pct > 100) pct = 100;
  pct_ = pct;
  guard_ = false;
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

namespace ledfx {
volatile float otaProgress = NAN;
}

// Couleur de fond : qualité de l'air, ou blanc qui respire sans verdict.
uint32_t LedStrip::baseColor(uint32_t now, uint8_t v) const {
  if (isnan(cur_)) {
    float b = 0.15f + 0.10f * sinf(now / 1000.0f * 2 * PI / 4.0f);
    uint8_t w = (uint8_t)(v * b);
    return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(w, w, w));
  }
  // Au-delà de la note 2, pulsation lente pour attirer l'attention
  float k = 1;
  if (cur_ >= 3.5f) k = 0.7f + 0.3f * sinf(now / 1000.0f * 2 * PI / 3.0f);
  uint16_t h = (uint16_t)(aircolor::hue(cur_) / 360.0f * 65535.0f);
  return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(h, 255, (uint8_t)(v * k)));
}

void LedStrip::tick() {
  if (!ready) return;
  const uint32_t now = millis();
  if (now - lastShow_ < 30) return;  // ~33 images/s
  lastShow_ = now;
  if (wantCount_ != count_) {
    // Nouveau nombre de LED : tout éteindre, puis renvoyer la couleur.
    show(0, LED_COUNT_MAX);
    count_ = wantCount_;
    lastColor_ = 0xFFFFFFFF;
  }
  const uint16_t n = count_;
  const float dt = (now - lastMs_) / 1000.0f;
  lastMs_ = now;

  // Démarrage doux : après l'animation d'accueil, la couleur de l'air monte
  // en 2,7 s (rien de fort pendant le pic de courant du démarrage du Wi-Fi).
  const uint32_t up = now - startMs_;
  const float ramp = up < 5300 ? 0 : (up < 8000 ? (up - 5300) / 2700.0f : 1);
  const bool off = pct_ == 0 || guard_ || (nightOff_ && isNight());
  const uint8_t v = off ? 0 : (uint8_t)(pct_ * 255 / 100 * ramp);
  // Luminosité des animations : celle réglée, au moins 15 %.
  const uint8_t vf = (uint8_t)((pct_ < 15 ? 15 : pct_) * 255 / 100);

  // Fondu : la position rejoint la cible avec une constante de temps de
  // LED_FADE_S secondes (les mesures sont déjà moyennées). Quand la note
  // change, la nouvelle couleur se propage d'un bout à l'autre de la bande.
  const float target = target_;
  if (isnan(target)) {
    cur_ = NAN;
    level_ = -1;
  } else {
    int8_t lvl = (int8_t)fminf(3, floorf(target));
    bool clear = level_ < 0 || fabsf(target - (lvl > level_ ? lvl : level_)) > 0.1f;
    if (lvl != level_ && clear) {
      if (level_ >= 0 && v) {
        sweepOld_ = baseColor(now, v);
        sweepStart_ = now;
      }
      level_ = lvl;
      cur_ = target;
    } else if (isnan(cur_)) {
      cur_ = target;
    } else {
      cur_ += (target - cur_) * (1 - expf(-dt / LED_FADE_S));
    }
  }

  const uint32_t base = v ? baseColor(now, v) : 0;
  for (uint16_t i = 0; i < n; i++) px[i] = base;
  bool fx = false;
  if (v && !isnan(cur_) && cur_ < 3.5f) {
    // Lumière vivante, très discrète : toute la bande respire doucement
    // (50 à 100 % de lumière, rythme d'environ 8 s, irrégulier) et chaque LED
    // dérive lentement de quelques degrés de teinte autour de la couleur de
    // l'air, à son propre rythme (ondes de 7 et 19 s décalées le long de la
    // bande) : des nuances qui bougent, jamais une LED qui change de couleur.
    // Calcul en lumière réelle (gamma 2,5) ; le reste d'arrondi passe à la LED
    // voisine pour éviter les paliers à faible luminosité.
    const float t = now / 1000.0f;
    const float h0 = aircolor::hue(cur_);
    const float g = 0.75f + 0.25f * (0.7f * sinf(2 * PI * t / 8) + 0.3f * sinf(2 * PI * t / 13));
    // Plafond de courant calculé sur le haut de la respiration : appliqué image
    // par image, il aplatissait la respiration à forte luminosité.
    float cap = 1;
    {
      const uint32_t c0 = Adafruit_NeoPixel::ColorHSV((uint16_t)(h0 / 360.0f * 65535.0f), 255, v);
      float sum = 0;
      for (int k = 0; k < 3; k++) {
        const float y = ((c0 >> (16 - 8 * k)) & 0xFF) / 255.0f;
        sum += 255 * y * y * sqrtf(y);
      }
      const float ma = sum * n * 20 / 255;
      if (ma > LED_MAX_MA * 0.95f) cap = LED_MAX_MA * 0.95f / ma;
    }
    float err[3] = {0, 0, 0};
    for (uint16_t i = 0; i < n; i++) {
      const float x = (float)i / n;
      const float dh = 25 * (0.6f * sinf(2 * PI * (t / 7 + x * 1.7f)) + 0.4f * sinf(2 * PI * (t / 19 - x * 2.3f + 0.4f)));
      const float m = g * (0.94f + 0.06f * sinf(2 * PI * (t / 11 + x * 1.1f)));
      const uint16_t h = (uint16_t)((h0 + dh + 360) / 360.0f * 65535.0f);
      const uint32_t c = Adafruit_NeoPixel::ColorHSV(h, 255, v);
      uint32_t o = 0;
      for (int k = 0; k < 3; k++) {
        const float y = ((c >> (16 - 8 * k)) & 0xFF) / 255.0f;
        const float want = 255 * y * y * sqrtf(y) * m * cap + err[k];
        const float q = fminf(255, fmaxf(0, roundf(want)));
        err[k] = want - q;
        o |= (uint32_t)q << (16 - 8 * k);
      }
      px[i] = o;
    }
    fx = true;
  }

  const float ota = ledfx::otaProgress;
  if (isnan(ota)) otaLit_ = -1;
  if (!guard_ && !isnan(ota)) {
    // Mise à jour : barre bleue qui se remplit. Une image seulement quand une
    // LED de plus s'allume : pendant l'écriture en flash, des envois à
    // 33 images/s ralentissaient l'envoi jusqu'au watchdog (0.9.13).
    const int16_t lit = (int16_t)(ota * n);
    if (lit == otaLit_) return;
    otaLit_ = lit;
    const uint32_t blue = Adafruit_NeoPixel::gamma32(rgb(0, vf / 4, vf));
    for (uint16_t i = 0; i < n; i++) px[i] = i < lit ? blue : 0;
    fx = true;
  } else if (!guard_ && (int32_t)(findUntil_ - now) > 0) {
    // Trouver la station : pulsation douce blanc-bleu, une par seconde
    const float k = 0.5f - 0.5f * cosf(now / 1000.0f * 2 * PI);
    const uint32_t c = scale(Adafruit_NeoPixel::gamma32(rgb(vf * 3 / 4, vf * 3 / 4, vf)), k);
    for (uint16_t i = 0; i < n; i++) px[i] = c;
    fx = true;
  } else if (!off && up >= 300 && up < 5300) {
    // Démarrage : les couleurs de l'échelle de l'air (vert, jaune, orange,
    // rouge) défilent lentement, avec une montée et une descente douces.
    const float t = (up - 300) / 1000.0f;
    const float env = fminf(1, fminf(t, 5 - t));
    const uint8_t vb = (uint8_t)(pct_ * 255 / 100 * env);
    for (uint16_t i = 0; i < n; i++) {
      float p = fmodf((float)i / n * 4 + t * 0.6f, 8);
      if (p > 4) p = 8 - p;
      uint16_t h = (uint16_t)(aircolor::hue(p) / 360.0f * 65535.0f);
      px[i] = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(h, 255, vb));
    }
    fx = true;
  } else if (v) {
    if (now - sweepStart_ < 1500 && sweepStart_) {
      // Nouvelle note : la couleur avance d'un bout à l'autre
      const float edge = (now - sweepStart_) / 1500.0f * (n + 3);
      for (uint16_t i = 0; i < n; i++) px[i] = mix(sweepOld_, base, (edge - i) / 3);
      fx = true;
    }
    // Fondu de 3 s à l'entrée et à la sortie de l'aération
    airK_ = airing_ ? fminf(1, airK_ + dt / 3) : fmaxf(0, airK_ - dt / 3);
    if (airK_ > 0) {
      // Aération : houle bleue lente et large, une respiration toutes les 6 s
      // sur chaque LED, qui avance doucement le long de la bande.
      const float t = (now - waveStart_) / 6000.0f;
      const uint32_t air = Adafruit_NeoPixel::gamma32(rgb(0, vf / 2, vf));
      const float k = airK_ * airK_ * (3 - 2 * airK_) * 0.75f;
      for (uint16_t i = 0; i < n; i++) {
        const float w = 0.5f - 0.5f * cosf(2 * PI * ((float)i / (n * 1.5f) - t));
        px[i] = mix(px[i], air, k * w * w * (3 - 2 * w));
      }
      fx = true;
    } else if (pmActive_) {
      // Mesure des poussières : un point plus clair glisse d'un bout à l'autre
      const float t = fmodf(now / 4000.0f, 2);
      const float c = (t < 1 ? t : 2 - t) * (n - 1);
      const uint32_t hi = mix(base, rgb(v, v, v), 0.5f);
      for (uint16_t i = 0; i < n; i++) px[i] = mix(px[i], hi, bump(i, c, 3));
      fx = true;
    }
  }

  // Limite de courant : environ 20 mA par canal à pleine valeur.
  uint32_t sum = 0;
  for (uint16_t i = 0; i < n; i++) sum += ((px[i] >> 16) & 0xFF) + ((px[i] >> 8) & 0xFF) + (px[i] & 0xFF);
  const uint32_t ma = sum * 20UL / 255UL;
  if (ma > LED_MAX_MA)
    for (uint16_t i = 0; i < n; i++) px[i] = scale(px[i], (float)LED_MAX_MA / ma);

  // Sans animation : une image seulement si la couleur change (la bande la
  // garde), et une fois toutes les 5 s au cas où un parasite l'aurait brouillée.
  if (!fx) {
    if (!fxShown_ && px[0] == lastColor_ && now - lastSent_ < 5000) return;
    lastColor_ = n ? px[0] : 0;
  }
  fxShown_ = fx;
  lastSent_ = now;
  for (uint16_t k = 0; k < n; k++) {
    grb[k * 3] = (px[k] >> 8) & 0xFF;
    grb[k * 3 + 1] = (px[k] >> 16) & 0xFF;
    grb[k * 3 + 2] = px[k] & 0xFF;
  }
  send(n);
}
