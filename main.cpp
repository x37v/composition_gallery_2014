#include <iostream>
#include <lo/lo.h>
#include <vector>
#include <mutex>
#include <chrono>
#include <random>
#include "osc_server.h"

using std::cout;
using std::endl;

typedef std::chrono::steady_clock clk;
typedef std::chrono::duration<int,std::milli> milliseconds_type;
typedef std::uniform_int_distribution<int> uni_int_distro;

int formant_center = 0;
clk::duration formant_period = milliseconds_type(1000);
clk::duration osc_period = milliseconds_type(10);
uni_int_distro formant_rand;

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
}

int main(int argc, char * argv[]) {
  osc_server::start(10001);

  osc::setup("", "9001");
  osc::send("/nvca", 0.0);
  osc::send("/tvca", 0.0);
  osc::send("/bvca", 0.0);

  //osc::send("/nvca", 0.5);
  osc::send("/npan", 0.5);

  clk::time_point next_osc = clk::now();
  clk::time_point last_update = clk::now();

  osc_server::with_float("/formant_period", [&] (float f) {
    formant_period = milliseconds_type(1 + static_cast<int>(f * 500.0));
  });

  osc_server::with_float("/formant_center", [&] (float f) {
      formant_center = static_cast<float>(vaddr.size()) * f;
  });

  osc_server::with_float("/formant_rand_range", [&] (float f) {
      int top = static_cast<int>(static_cast<float>(vaddr.size()) * f) - 1;
      formant_rand = uni_int_distro(0, top >= 0 ? top : 0);
  });

  osc_server::with_float("/noise_volume", [&] (float f) {
    osc::send("/nvca", f);
  });

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  formant_rand = uni_int_distro(0, 0);

  while (1) {
    clk::time_point n = clk::now();
    if (last_update + formant_period < n) {
      int index = (formant_center + formant_rand(generator)) % vaddr.size();
      osc::send(vaddr[index]);
      last_update = n;
    }
    if (next_osc < n) {
      osc_server::process();
      //next_osc += osc_period;
      next_osc = n + osc_period;
    }
  }

  return 0;
}
