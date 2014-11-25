#ifndef ENVELOPE_H
#define ENVELOPE_H

class Envelope {
  public:
    enum mode_t { LINEAR, TRIANGLE };

    Envelope(mode_t m = LINEAR, float inc = 0.01f);

    void mode(mode_t m);
    mode_t mode() const;

    void increment(float v);
    float increment() const;

    void restart();
    bool complete() const;

    bool update();
    float value() const;
  private:
    mode_t mMode = LINEAR;
    float mIndex = 0.0f;
    float mIncrement = 0.01f;
};
#endif
