#include "envelope.h"
#include <cmath>
#include <algorithm>

Envelope::Envelope(mode_t m, float inc) :
  mMode(m), mIncrement(inc) { }

void Envelope::mode(mode_t m) { mMode = m; }
Envelope::mode_t Envelope::mode() const { return mMode; }

void Envelope::increment(float v) { mIncrement = fabs(v); }
float Envelope::increment() const { return mIncrement; }

void Envelope::restart() { mIndex = 0.0f; mActive = true; }
void Envelope::set_complete() { mIndex = 1.0f; }
bool Envelope::complete() const { return mIndex >= 1.0f; }
bool Envelope::active() const { return mActive; }

bool Envelope::update() { 
  mActive = mIndex < 1.0f;
  mIndex = std::min(mIncrement + mIndex, 1.0f);
  return complete();
}

float Envelope::value() const {
  switch (mMode) {
    case TRIANGLE:
      if (mIndex <= 0.5f)
        return mIndex * 2.0f;
      else
        return std::max(0.0f, 2.0f - 2.0f * mIndex);
    case HALF_SIN:
      if (mIndex >= 1.0f)
        return 0.0f;
      return sin(M_PI * mIndex);
    case QUARTER_SIN:
      if (mIndex >= 1.0f)
        return 1.0f;
      return sin(0.5 * M_PI * mIndex);
    case RAMP_UP:
      return mIndex;
    case RAMP_DOWN:
      return 1.0f - mIndex;
  }
}
