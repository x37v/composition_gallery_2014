#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <mutex>
#include <map>
#include <deque>
#include <functional>

#include "lo/lo.h"

namespace osc_server {
  std::mutex osc_mutex; 

  std::map<std::string, std::function<void(float)> > func_map_f;
  std::map<std::string, std::function<void(int)> > func_map_i;

  std::deque<std::pair<std::string, float> > msg_lst_f;
  std::deque<std::pair<std::string, int> > msg_lst_i;

  lo_server_thread st;
  int done = 0;

  void error(int num, const char *m, const char *path);

  int generic_handler(const char *path, const char *types, lo_arg ** argv,
      int argc, void *data, void *user_data);

  void start(int port) {
    st = lo_server_thread_new(std::to_string(port).c_str(), error);

    /* add method that will match any path and args */
    lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);

    lo_server_thread_start(st);
  }

  void stop() {
    lo_server_thread_free(st);
  }

  void process() {
    std::deque<std::pair<std::string, float> > copy_f;
    std::deque<std::pair<std::string, int> > copy_i;
    {
      std::lock_guard<std::mutex> lock(osc_mutex);
      copy_f = msg_lst_f;
      copy_i = msg_lst_i;
      msg_lst_i.clear();
      msg_lst_f.clear();
    }

    for (auto msg: copy_f) {
      auto it = func_map_f.find(msg.first);
      if (it != func_map_f.end()) {
        it->second(msg.second);
      }
    }
    for (auto msg: copy_i) {
      auto it = func_map_i.find(msg.first);
      if (it != func_map_i.end()) {
        it->second(msg.second);
      }
    }
  }

  void with_float(std::string addr, std::function<void(float)> func) {
    func_map_f[addr] = func;
  }

  void with_int(std::string addr, std::function<void(int)> func) {
    func_map_i[addr] = func;
  }

  void error(int num, const char *msg, const char *path) {
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
  }

  int generic_handler(const char *path, const char *types, lo_arg ** argv,
      int argc, void *data, void *user_data)
  {
    if (argc == 1) {
      switch (types[0]) {
        case LO_FLOAT:
          {
            std::lock_guard<std::mutex> lock(osc_mutex);
            msg_lst_f.push_back({std::string(path), argv[0]->f});
          }
          break;
        case LO_INT32:
          {
            std::lock_guard<std::mutex> lock(osc_mutex);
            msg_lst_i.push_back({std::string(path), argv[0]->i});
          }
          break;
        default:
          return 1;
      }
      return 0;
    }
    return 1;
  }
}
