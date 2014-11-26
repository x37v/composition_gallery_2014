#include <iostream>
#include <lo/lo.h>
#include <vector>
#include <array>
#include <mutex>
#include <chrono>
#include <random>
#include <unistd.h>
#include "osc_server.h"
#include "envelope.h"
#include "RtMidi.h"
#include <memory>
#include <signal.h>

#define NUM_LEDS 6

using std::cout;
using std::endl;

bool done = false;

void sigint_handler(int s){
  done = true;
}

enum {
  TONE = 0,
  NOISE = 1,
  BASS = 2
};

enum preformance_mode_t {
  FREE,
  NOTES
};


//primes
std::vector<float> tone_offset = {
  0.0,
  //1.0 / 2.0,
  1.0 / 3.0,
  1.0 / 5.0,
  1.0 / 7.0,
  1.0 / 11.0,
  1.0 / 13.0,
  1.0 / 17.0,
};

std::vector<Envelope> led_envs;

/*
   std::vector<float> tone_offset = {
   0.0,
   0.840188,
   0.394383,
   0.783099,
   0.79844,
   0.911647
   };
*/

preformance_mode_t performance_mode = NOTES;

typedef std::chrono::steady_clock clk;
typedef std::chrono::duration<int,std::milli> milliseconds_type;
typedef std::uniform_real_distribution<double> uni_real_distro;

float formant_center = 0.0f;
float formant_range = 0.0f;

float volume[3] = {0.0f, 0.0f, 0.1};

clk::duration formant_period = milliseconds_type(1000);
clk::duration led_period = milliseconds_type(40);
clk::duration pan_period = milliseconds_type(80);

std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
uni_real_distro real_rand = uni_real_distro(0, 1.0);

float noise_pan = 0.5f;
float tone_pan = 0.5f;

float noise_pan_incr = 0.0f;
float tone_pan_incr = 0.0f;
float tone_pan_spread = 1.0f;
float tone_freq_spread = 0.0f;

const float pan_mult = 0.05f;

const std::vector<std::string> vaddr = { "/vowel_ee", "/vowel_ih", "/vowel_eh", "/vowel_ae", "/vowel_a", "/vowel_oo", "/vowel_uh", "/vowel_o", "/vowel_ah"};

namespace osc {
  std::mutex osc_mutex; 
  lo_address osc_address = nullptr;
  
  void setup(std::string host, std::string port) {
    if (osc_address)
      lo_address_free(osc_address);
    osc_address = lo_address_new(host.size() ? host.c_str() : NULL, port.c_str());
  }

  void send(const std::string addr) {
    std::lock_guard<std::mutex> lock(osc_mutex);
    lo_send(osc_address, addr.c_str(), "");
  }

  void send(const std::string addr, float v) {
    std::lock_guard<std::mutex> lock(osc_mutex);
    lo_send(osc_address, addr.c_str(), "f", v);
  }

  void send_led(int index, float h, float s, float l) {
    std::lock_guard<std::mutex> lock(osc_mutex);
    //l = std::min(1.0f, std::max(0.0f, static_cast<float>(std::sin(l * M_PI / 2.0))));
    lo_send(osc_address, ("/led" + std::to_string(index + 1)).c_str(), "fff", h, s, l);
  }
}

float frand() {
  return real_rand(generator);
}

int map_vca(int osc) {
  switch (osc) {
    case 0:
      return 3;
    case 1:
      return 1;
    case 2:
      return 2;
    default:
      return osc + 1;
  }
}

void set_volumes(float master) {
  for (int i = 1; i < 6; i++) {
    float start = static_cast<float>(i) / 12.0f;
    float v = master - start;
    if (start > 0) {
      v /= start;
    } else {
      v *= 2.0;
    }
    v = std::min(1.0f, std::max(0.0f, v));
    osc::send("/vca" + std::to_string(map_vca(i)), v);
  }
}

bool was_note = false;

std::vector<std::vector<float>> leds;
int led_offset = 0;
float color = 0;

void draw_leds() {
#if 0
  if (was_note) {
    led_offset = frand() * static_cast<float>(NUM_LEDS);
    for (int i = 0; i < NUM_LEDS; i++) {
      int l = (led_offset + i) % NUM_LEDS;
      if (leds[l][2] == 0.0f) {
        leds[l][0] = frand();
        leds[l][1] = 1.0;
        leds[l][2] = std::max(0.0f, volume[TONE] - frand() * 0.1f);
        break;
      }
    }
  }
  for (int i = 0; i < NUM_LEDS; i++) {
    osc::send_led(i, leds[i][0], leds[i][1], leds[i][2]);
    leds[i][2] = std::max(0.0f, leds[i][2] - 0.047f);
  }
#else

  if (was_note && led_envs.front().complete() && led_envs.back().complete()) {
    led_offset = 0;
    color = frand();
    for (int i = 0; i < NUM_LEDS / 2; i++) {
      leds[i][0] = color;
      leds[i][1] = 1.0;
      leds[i][2] = 0.0f;
      led_envs[i].restart();
    }
  } else {
    led_offset++;
  }

  if (led_offset == 16) {
    for (int i = 3; i < NUM_LEDS; i++) {
      leds[i][0] = color;
      leds[i][1] = 1.0;
      leds[i][2] = 0.0;
      led_envs[i].restart();
    }
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    osc::send_led(i, leds[i][0], leds[i][1], led_envs[i].value());
    led_envs[i].update();
  }
#endif
  was_note = false;
}

void note(int& index, clk::time_point& next_note) {
  next_note = clk::now() + milliseconds_type(static_cast<int>(800.0 * frand()));
  int m = index % 2;
  if (m == 0) {
    osc::send("/tvca", volume[TONE]);
    osc::send("/nvca", volume[NOISE]);
    osc::send("/bvca", volume[BASS]);
    was_note = true;
  } else if (m == 1) {
    osc::send("/tvca", 0.0);
    osc::send("/nvca", 0.0);
    osc::send("/bvca", 0.0);
  } else {
    //index = 0;
    //return;
  }
  index++;
}

void set_freqs() {
  for (int i = 1; i < 6; i++) {
    float v = 0.05 * frand() + tone_offset[i] * tone_freq_spread;
    osc::send("/t" + std::to_string(i + 1), v);
  }
}

namespace midi {
  void process();
  void open();
}

//white = saturation 0, value = 1, hue = 1

int main(int argc, char * argv[]) {
  midi::open();
  //osc::setup("192.168.0.100", "9001"); //main patch
  osc::setup("", "1888"); //jason's mockup, with route through

  osc::send("/remote");

  sleep(1);
  
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = sigint_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);

  osc::send("/vca" + std::to_string(map_vca(0)), 1.0);

  osc::send("/nvca", 0.0f);
  osc::send("/tvca", 0.0f);
  osc::send("/bvca", 0.0f);

  osc::send("/thpf", 0.0);
  osc::send("/tlpf", 127.0);

  osc::send("/formanttime", 10.0);

  osc::send("/t1", 65);
  osc::send("/bfreq", 92);

  for (int i = 0; i < NUM_LEDS; i++) {
    led_envs.push_back(Envelope(Envelope::TRIANGLE));
    leds.push_back({0.0f, 0.0f, 0.0f});
    osc::send_led(i, 1.0, 0, 1.0);
  }

  set_freqs();
  set_volumes(0.0);

  osc::send("/npan", tone_pan);

  int index_last = -1;

  clk::time_point next_pan = clk::now();
  clk::time_point next_note = clk::now();
  clk::time_point last_formant = clk::now();
  clk::time_point next_led = clk::now();

  int note_index = 0;

  while (!done) {
    midi::process();

    clk::time_point n = clk::now();
    if (last_formant + formant_period < n) {
      int index = static_cast<int>(round(formant_center + frand() * formant_range)) % vaddr.size();
      if (index != index_last)
        osc::send(vaddr[index]);
      index_last = index;
      last_formant = n;
    }

    if (next_note < n) {
      if (performance_mode == NOTES)
        note(note_index, next_note);
    }

    if (next_led < n) {
      draw_leds();
      next_led = n + led_period;
    }

    if (next_pan < n) {
      next_pan = n + pan_period;

      if (true) {
        tone_pan += tone_pan_incr;
        while (tone_pan < 0)
          tone_pan += 1;
        while (tone_pan > 1)
          tone_pan -= 1;
        for (int i = 0; i < 6; i++) {
          float p = tone_pan * 360;
          float off = static_cast<float>(i) * tone_pan_spread * 60.0;
          p += off;
          while (p > 360)
            p -= 360;
          while (p < 0)
            p += 360;
          osc::send("/pan" + std::to_string(i + 1), p);
        }
      }

      if (noise_pan_incr) {
        noise_pan += noise_pan_incr;
        while (noise_pan < 0)
          noise_pan += 1;
        while (noise_pan > 1)
          noise_pan -= 1;
        osc::send("/npan", noise_pan * 360);
      }
    }
  }

  osc::send("/nvca", 0.0f);
  osc::send("/tvca", 0.0f);
  osc::send("/bvca", 0.0f);

  sleep(1);

  return 0;
}

namespace midi {
  const std::string midi_name = "Bitstream 3X";
  std::shared_ptr<RtMidiIn> midiin;

#define CC 0xB0
#define NOTEON 0x90
#define NOTEOFF 0x80

  void open() {
    try {
      midiin = std::make_shared<RtMidiIn>();
    } catch (RtError &error) {
      throw std::runtime_error(error.what());
    }

    // Check inputs.
    unsigned int nPorts = midiin->getPortCount();
    for (unsigned int i=0; i<nPorts; i++) {
      try {
        std::string name = midiin->getPortName(i);
        if (name.find(midi_name) != std::string::npos) {
          midiin->openPort(i);
          return;
        }
      } catch (RtError &error) {
        throw std::runtime_error(error.what());
      }
    }
    throw std::runtime_error("couldn't connect to midi with port name: " + midi_name);
  }

  void cc(uint8_t chan, uint8_t num, uint8_t val) {
    cout << "cc " << (int)num << ": " << (int)val << " chan: " << (int)chan << endl;
    float f = static_cast<float>(val) / 127.0;
    switch (num) {
      case 13:
        break;
      case 21:
        break;
      case 29:
        break;
      case 37:
        break;
      case 45:
        volume[BASS] = f;
        if (performance_mode == FREE)
          osc::send("/bvca", f);
        break;

      case 14:
        if (val > 60 && val < 70)
          noise_pan_incr = 0;
        else
          noise_pan_incr = pan_mult * (static_cast<float>(val - 64) / 64.0);
        break;
      case 22:
        osc::send("/formanttime", f * 100.0);
        break;
      case 30:
        formant_center = static_cast<float>(vaddr.size()) * f;
        break;
      case 38:
        formant_range = static_cast<float>(vaddr.size()) * f;
        break;
      case 46:
        volume[NOISE] = f;
        if (performance_mode == FREE)
          osc::send("/nvca", f);
        break;

        //last column
      case 15:
        if (val > 60 && val < 70)
          tone_pan_incr = 0.0f;
        else
          tone_pan_incr = pan_mult * (static_cast<float>(val - 64) / 64.0);
        break;
      case 23:
        tone_pan_spread = f;
        break;
      case 31:
        tone_freq_spread = f * 24;
        set_freqs();
        break;
      case 39:
        set_volumes(f);
        break;
      case 47:
        volume[TONE] = f;
        if (performance_mode == FREE)
          osc::send("/tvca", f);
        break;

        //xfade
      case 48:
        formant_period = milliseconds_type(1 + static_cast<int>(f * 500.0));
        break;
    }
  }

  void note(bool down, uint8_t chan, uint8_t num, uint8_t vel) {
    cout << "note" << (down ? "on " : "off ") << (int)num << ": " << (int)vel << " chan: " << (int)chan << endl;
  }

  void process() {
    std::vector<unsigned char> message;
    do {
      midiin->getMessage(&message);
      if (message.size() == 0)
        return;

      if (message.size() == 3) {
        uint8_t status = (message[0] & 0xF0);
        uint8_t chan = (message[0] & 0x0F);
        uint8_t num = message[1];
        uint8_t val = message[2];
        switch (status) {
          case CC:
            cc(chan, num, val);
            break;
          case NOTEON:
            note(true, chan, num, val);
            break;
          case NOTEOFF:
            note(false, chan, num, val);
            break;
        }
      }
    } while (1);
  }
}

