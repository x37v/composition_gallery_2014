#ifndef OSC_SERVER_H
#define OSC_SERVER_H

namespace osc_server {
  void start(int port);
  void stop();
  //execute osc stuff
  void process();

  //will happen in ::process calling thread
  void with_float(std::string addr, std::function<void(float)> func);
  void with_int(std::string addr, std::function<void(int)> func);
}

#endif
