#ifndef ENVELOPE_H
#define ENVELOPE_H

class Envelope {
  public:
    enum mode_t { RAMP_UP, RAMP_DOWN, TRIANGLE, HALF_SIN, QUARTER_SIN };

    Envelope(mode_t m = RAMP_UP, float inc = 0.01f);

    void mode(mode_t m);
    mode_t mode() const;

    void increment(float v);
    float increment() const;

    void restart();
    bool complete() const;
    bool active() const;

    bool update();
    float value() const;
  private:
    mode_t mMode = RAMP_UP;
    float mIndex = 0.0f;
    float mIncrement = 0.01f;
};
#endif
