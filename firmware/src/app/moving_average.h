#pragma once
#include <math.h>
#include <stddef.h>

// Moyenne mobile simple sur N échantillons (tampon circulaire).
template <size_t N>
class MovingAverage {
 public:
  float add(float v) {
    if (isnan(v)) return value();
    if (count_ == N) sum_ -= buf_[head_];
    else count_++;
    buf_[head_] = v;
    sum_ += v;
    head_ = (head_ + 1) % N;
    return value();
  }
  float value() const { return count_ ? sum_ / count_ : NAN; }
  void reset() { count_ = head_ = 0; sum_ = 0; }

 private:
  float buf_[N] = {};
  size_t head_ = 0, count_ = 0;
  float sum_ = 0;
};
