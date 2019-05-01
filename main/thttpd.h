#ifndef MAIN_THTTPD_H_
#define MAIN_THTTPD_H_

#include <memory>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "base/err.h"
#include "base/mpsc-queue.h"
#include "main/config.h"
#include "main/request-handler.h"
#include "main/thread-pool.h"

class Thttpd {
 public:
  Thttpd(const Thttpd&) = delete;
  Thttpd& operator=(const Thttpd&) = delete;

  static Result<std::unique_ptr<Thttpd>> Create(const Config& config);

  // Blocks.
  ABSL_MUST_USE_RESULT Result<void> Start();

  const Config& config() const { return config_; }

 private:
  friend class RequestHandler;

  explicit Thttpd(const Config& config);

  // Friend methods:
  void NotifySocketClosed(int fd);

  void NotifyEvent(absl::string_view event);
  void AcceptNewClient(int listen_fd, int epoll_fd);

  // Returns true if the main loop should exit.
  bool HandleEvents(int event_fd);
  void HandleClient(int fd, uint32_t epoll_events);

  const Config config_;

  ThreadPool thread_pool_;
  absl::flat_hash_map<int, std::shared_ptr<RequestHandler>> conn_fd_to_handler_;

  // Used for notifying the main loop of events.
  ScopedFd event_write_fd_;
  MpscQueue<int> closed_fds_;
};

#endif  // MAIN_THTTPD_H_
