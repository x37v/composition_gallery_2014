#include "envelope.h"
#include <cmath>
#include <algorithm>

Envelope::Envelope(mode_t m, float inc) :
  mMode(m), mIncrement(inc) { }

void Envelope::mode(mode_t m) { mMode = m; }
Envelope::mode_t Envelope::mode() const { return mMode; }

void Envelope::increment(float v) { mIncrement = fabs(v); }
float Envelope::increment() const { return mIncrement; }

void Envelope::restart() { mIndex = 0.0f; }
bool Envelope::complete() const { return mIndex >= 1.0f; }

bool Envelope::update() { 
  mIndex += mIncrement;
  return complete();
}

float Envelope::value() const {
  switch (mMode) {
    case TRIANGLE:
      if (mIndex <= 0.5f)
        return mIndex * 2.0f;
      else
        return std::min(0.0f, 2.0f - 2.0f * mIndex);
    case LINEAR:
    default:
      return std::min(1.0f, mIndex);
  }
}
