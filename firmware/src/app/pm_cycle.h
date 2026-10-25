#pragma once
#include <Arduino.h>

#include "../config.h"
#include "../hal/sds011.h"

// Ordonnanceur du SDS011 : veille -> préchauffage -> mesure -> veille.
// Préserve le laser et le ventilateur en ne les faisant tourner qu'une
// fraction du temps. Une période de 0 = fonctionnement continu.
class PmCycle {
 public:
  enum class State : uint8_t { Continuous, Sleeping, Warming, Measuring };

  explicit PmCycle(SDS011& sensor) : s_(sensor) {}

  void begin(uint32_t periodS) { setPeriod(periodS); }

  void setPeriod(uint32_t periodS) {
    periodMs_ = periodS * 1000UL;
    if (periodMs_ == 0) {
      s_.wake();
      state_ = State::Continuous;
      lastFrames_ = s_.frameCount();
    } else {
      startCycle(millis());
    }
  }

  // Lance un cycle de mesure tout de suite.
  void measureNow() {
    if (state_ != State::Continuous) startCycle(millis());
  }

  // À appeler souvent depuis la tâche capteurs.
  void update() {
    s_.poll();
    uint32_t now = millis();
    uint32_t frames = s_.frameCount();
    bool newFrame = frames != lastFrames_;
    lastFrames_ = frames;

    switch (state_) {
      case State::Continuous:
        if (newFrame) {
          s_.latest(pm25_, pm10_);
          fresh_ = true;
          present_ = true;
          lastResultMs_ = now;
        } else if (now - lastResultMs_ > 5000) {
          present_ = false;
        }
        break;

      case State::Sleeping:
        if ((int32_t)(now - nextWakeMs_) >= 0) startCycle(now);
        break;

      case State::Warming:
        if (now - stateStartMs_ >= PM_WARMUP_MS) {
          state_ = State::Measuring;
          stateStartMs_ = now;
          sum25_ = sum10_ = 0;
          n_ = 0;
        }
        break;

      case State::Measuring:
        if (newFrame) {
          float a, b;
          if (s_.latest(a, b)) {
            sum25_ += a;
            sum10_ += b;
            n_++;
          }
        }
        if (now - stateStartMs_ >= PM_MEASURE_MS) {
          present_ = n_ > 0;
          if (n_ > 0) {
            pm25_ = sum25_ / n_;
            pm10_ = sum10_ / n_;
            fresh_ = true;
            lastResultMs_ = now;
          }
          s_.sleep();
          state_ = State::Sleeping;
          nextWakeMs_ = cycleStartMs_ + periodMs_;
          if ((int32_t)(nextWakeMs_ - now) < 0) nextWakeMs_ = now + periodMs_;
        }
        break;
    }
  }

  // Retourne true une seule fois par nouvelle mesure.
  bool takeFresh(float& pm25, float& pm10) {
    if (!fresh_) return false;
    fresh_ = false;
    pm25 = pm25_;
    pm10 = pm10_;
    return true;
  }

  bool isPresent() const { return present_; }
  State state() const { return state_; }
  uint32_t periodS() const { return periodMs_ / 1000; }
  uint32_t secondsToNext() const {
    if (state_ != State::Sleeping) return 0;
    int32_t d = (int32_t)(nextWakeMs_ - millis());
    return d > 0 ? d / 1000 : 0;
  }
  static const char* stateName(State s) {
    switch (s) {
      case State::Continuous: return "continuous";
      case State::Sleeping: return "sleep";
      case State::Warming: return "warmup";
      case State::Measuring: return "measure";
    }
    return "?";
  }

 private:
  void startCycle(uint32_t now) {
    s_.wake();
    state_ = State::Warming;
    stateStartMs_ = cycleStartMs_ = now;
  }

  SDS011& s_;
  State state_ = State::Sleeping;
  uint32_t periodMs_ = 0;
  uint32_t stateStartMs_ = 0, cycleStartMs_ = 0, nextWakeMs_ = 0;
  uint32_t lastFrames_ = 0, lastResultMs_ = 0;
  float sum25_ = 0, sum10_ = 0;
  uint16_t n_ = 0;
  float pm25_ = NAN, pm10_ = NAN;
  bool fresh_ = false, present_ = false;
};
