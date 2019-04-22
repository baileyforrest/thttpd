#ifndef MAIN_THTTPD_H_
#define MAIN_THTTPD_H_

#include <memory>

#include "absl/base/attributes.h"
#include "base/err.h"
#include "main/thread-pool.h"

class Thttpd {
 public:
  struct Config {
    uint16_t port = 0;
    int num_worker_threads = 0;  // If 0, will pick based on number of cores.
    int verbosity = 1;
  };

  Thttpd(const Thttpd&) = delete;
  Thttpd& operator=(const Thttpd&) = delete;

  static Result<std::unique_ptr<Thttpd>> Create(const Config& config);

  // Blocks.
  ABSL_MUST_USE_RESULT Result<void> Start();

 private:
  explicit Thttpd(const Config& config);

  void AcceptNewClient(int listen_fd, int epoll_fd) const;

  const uint16_t port_;
  const int verbosity_;
  ThreadPool thread_pool_;
};

#endif  // MAIN_THTTPD_H_
