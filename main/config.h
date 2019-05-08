#ifndef _MAIN_CONFIG_H_
#define _MAIN_CONFIG_H_

#include <cstdint>
#include <string>

struct Config {
  uint16_t port = 0;
  int num_worker_threads = 0;  // If 0, will pick based on number of cores.
  int verbosity = 1;
  std::string path_to_serve;
  size_t compression_cache_size = 1000ul * 1000 * 1000;
};

#endif  // _MAIN_CONFIG_H_
