#include <iostream>
#include <lo/lo.h>
#include <vector>

namespace osc {
  lo_address osc_address = nullptr;
  
  void setup(std::string host, std::string port) {
    if (osc_address)
      lo_address_free(osc_address);
    osc_address = lo_address_new(host.size() ? host.c_str() : NULL, port.c_str());
  }

  void send(const std::string addr) {
    lo_send(osc_address, addr.c_str(), "");
  }

  void send(const std::string addr, float v) {
    lo_send(osc_address, addr.c_str(), "f", v);
  }
}

const std::vector<std::string> vaddr = { "/vowel_ee", "/vowel_ih", "/vowel_eh", "/vowel_ae", "/vowel_a", "/vowel_oo", "/vowel_uh", "/vowel_o", "/vowel_ah"};

int main(int argc, char * argv[]) {
  osc::setup("", "9001");
  osc::send("/nvca", 0.0);
  osc::send("/tvca", 0.0);
  osc::send("/bvca", 0.0);

  osc::send("/nvca", 0.5);
  osc::send("/npan", 0.5);

  return 0;
}
