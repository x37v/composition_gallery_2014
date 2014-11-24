#include <iostream>
#include <lo/lo.h>
#include <vector>
#include <array>
#include <mutex>
#include <chrono>
#include <random>
#include "osc_server.h"

#define NUM_LEDS 6

using std::cout;
using std::endl;

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

preformance_mode_t performance_mode = NOTES;

typedef std::chrono::steady_clock clk;
typedef std::chrono::duration<int,std::milli> milliseconds_type;
typedef std::uniform_real_distribution<double> uni_real_distro;

float formant_center = 0.0f;
float formant_range = 0.0f;

float volume[3] = {0.0f, 0.0f, 0.1};

clk::duration formant_period = milliseconds_type(1000);
clk::duration osc_period = milliseconds_type(10);
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
    l = std::min(1.0f, std::max(0.0f, static_cast<float>(std::sin(l * M_PI / 2.0))));
    lo_send(osc_address, ("/led" + std::to_string(index + 1)).c_str(), "fff", h, s, l);
  }
}

float frand() {
  return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
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

void draw_leds() {
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
  was_note = false;
}

void note(int& index, clk::time_point& next_note) {
  next_note = clk::now() + milliseconds_type(static_cast<int>(200.0 * frand()));
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

int main(int argc, char * argv[]) {
  osc_server::start(10001);

  //osc::setup("", "9001"); //main patch
  osc::setup("", "1888"); //jason's mockup, with route through

  osc::send("/nvca", 0.0);
  osc::send("/tvca", 0.0);
  osc::send("/bvca", 0.0);

  osc::send("/thpf", 0.0);
  osc::send("/tlpf", 127.0);

  osc::send("/vca" + std::to_string(map_vca(0)), 0.0);

  osc::send("/t1", 64);
  osc::send("/bfreq", 92);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds.push_back({0.0f, 0.0f, 0.0f});
    osc::send_led(i, 0, 0, 0);
  }

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

  auto set_freqs = [&]{
    for (int i = 1; i < 6; i++) {
      float v = 0.05 * frand() + tone_offset[i] * tone_freq_spread;
      osc::send("/t" + std::to_string(i + 1), v);
    }
  };
  set_freqs();

  osc::send("/npan", tone_pan);


  osc_server::with_float("/formant_time", [&] (float f) {
    osc::send("/formanttime", f * 100.0);
  });

  osc_server::with_float("/formant_period", [&] (float f) {
    formant_period = milliseconds_type(1 + static_cast<int>(f * 500.0));
  });

  osc_server::with_float("/formant_center", [&] (float f) {
      formant_center = static_cast<float>(vaddr.size()) * f;
  });

  osc_server::with_float("/formant_rand_range", [&] (float f) {
      formant_range = static_cast<float>(vaddr.size()) * f;
  });

  osc_server::with_float("/noise_volume", [&] (float f) {
      volume[NOISE] = f;
      if (performance_mode == FREE)
        osc::send("/nvca", f);
  });

  osc_server::with_float("/tone_volume", [&] (float f) {
      volume[TONE] = f;
      if (performance_mode == FREE)
        osc::send("/tvca", f);
  });

  osc_server::with_float("/tone_pan_spread", [&] (float f) {
      tone_pan_spread = f;
  });

  osc_server::with_float("/tone_freq_spread", [&] (float f) {
      tone_freq_spread = f * 24;
      set_freqs();
  });

  osc_server::with_float("/tone_volume_spread", [&] (float f) {
      set_volumes(f);
  });

  osc_server::with_int("/tone_pan", [&] (int v) {
      if (v > 60 && v < 66)
        tone_pan_incr = 0.0f;
      else
        tone_pan_incr = pan_mult * (static_cast<float>(v - 64) / 64.0);
  });

  osc_server::with_int("/noise_pan", [&] (int v) {
      if (v > 60 && v < 66)
        noise_pan_incr = 0;
      else
        noise_pan_incr = pan_mult * (static_cast<float>(v - 64) / 64.0);
  });

  int index_last = -1;

  clk::time_point next_osc = clk::now();
  clk::time_point next_pan = clk::now();
  clk::time_point next_note = clk::now();
  clk::time_point last_formant = clk::now();
  clk::time_point next_led = clk::now();

  int note_index = 0;

  while (1) {
    clk::time_point n = clk::now();
    if (last_formant + formant_period < n) {
      int index = static_cast<int>(round(formant_center + frand() * formant_range)) % vaddr.size();
      if (index != index_last)
        osc::send(vaddr[index]);
      index_last = index;
      last_formant = n;
    }

    if (next_osc < n) {
      osc_server::process();
      //next_osc += osc_period;
      next_osc = n + osc_period;
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

  return 0;
}
