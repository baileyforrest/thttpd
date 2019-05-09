#include "main/thttpd.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>

#include "absl/base/macros.h"
#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "absl/types/span.h"
#include "base/logging.h"
#include "base/scoped-fd.h"

namespace {

constexpr int kListenBacklog = 128;
constexpr int kMaxEvents = 4096;

}  // namespace

// static
Result<std::unique_ptr<Thttpd>> Thttpd::Create(const Config& config_in) {
  Config config = config_in;
  if (config.num_worker_threads < 0) {
    return Err("Must specify a positive number of threads, or 0 to auto pick");
  }

  if (config.num_worker_threads == 0) {
    // TODO(bcf): Choose based number of cores.
    config.num_worker_threads = 16;
  }

  return absl::WrapUnique(new Thttpd(config));
}

Thttpd::Thttpd(const Config& config)
    : config_(config),
      thread_pool_(config.num_worker_threads),
      compression_cache_(config.compression_cache_size) {}

Result<void> Thttpd::Start() {
  ScopedFd listen_fd(
      socket(PF_INET6, SOCK_STREAM | SOCK_NONBLOCK, /*protocol=*/0));
  if (*listen_fd < 0) {
    return BuildPosixErr("socket failed");
  }

  {
    int opt = 1;
    if (setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
      return BuildPosixErr("socksockopt failed");
    }
  }

  {
    sockaddr_in6 addr = {
        .sin6_family = AF_INET6, .sin6_port = htons(config_.port),
    };
    addr.sin6_addr = in6addr_any;
    if (bind(*listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) <
        0) {
      return BuildPosixErr("bind failed");
    }

    if (listen(*listen_fd, kListenBacklog) < 0) {
      return BuildPosixErr("listen failed");
    }
  }

  ScopedFd epoll_fd(epoll_create1(/*flags=*/0));
  if (*epoll_fd < 0) {
    return BuildPosixErr("epoll_create1 failed");
  }

  {
    epoll_event event{
        .events = EPOLLIN,
    };
    event.data.fd = *listen_fd;
    if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, *listen_fd, &event) < 0) {
      return BuildPosixErr("epoll_ctl on listen_fd failed");
    }
  }

  int pipe_fds[2];
  if (pipe(pipe_fds) < 0) {
    return BuildPosixErr("pipe failed");
  }
  ScopedFd event_read_fd = ScopedFd(pipe_fds[0]);
  event_write_fd_ = ScopedFd(pipe_fds[1]);

  {
    // Set event_read_fd as nonblocking.
    int flags = fcntl(*event_read_fd, F_GETFL, 0);
    if (flags < 0) {
      return BuildPosixErr("fcntl(F_GETFL) on event_read_fd failed");
    }
    flags |= O_NONBLOCK;
    if (fcntl(*event_read_fd, F_SETFL, flags) < 0) {
      return BuildPosixErr("fcntl(F_SETFL) on event_read_fd failed");
    }

    // Add epoll event for event_read_fd.
    epoll_event event{
        .events = EPOLLIN,
    };
    event.data.fd = *event_read_fd;
    if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, *event_read_fd, &event) < 0) {
      return BuildPosixErr("epoll_ctl on event_read_fd failed");
    }
  }

  const auto events = absl::make_unique<std::array<epoll_event, kMaxEvents>>();

  LOG(INFO) << "Listening on port " << config_.port;
  while (true) {
    int num_fds =
        epoll_wait(*epoll_fd, events->data(), events->size(), /*timeout=*/-1);
    if (num_fds < 0) {
      return BuildPosixErr("epoll_wait failed");
    }

    for (auto& event : absl::MakeSpan(events->data(), num_fds)) {
      int fd = event.data.fd;
      if (fd == *listen_fd) {
        AcceptNewClient(*listen_fd, *epoll_fd);
      } else if (fd == *event_read_fd) {
        HandleEvents(*event_read_fd);
      } else {
        HandleClient(fd, event.events);
      }
    }
  }

  return {};
}

void Thttpd::NotifySocketClosed(int fd) {
  closed_fds_.Push(fd);
  NotifyEvent(absl::StrCat("socket closed: ", fd));
}

void Thttpd::NotifyEvent(absl::string_view event) {
  std::string str_event(event);
  str_event.push_back('\n');
  ssize_t ret = write(*event_write_fd_, str_event.data(), str_event.size());
  if (ret != static_cast<ssize_t>(str_event.size())) {
    LOG(ERR) << "Failed to write event: " << event;
  }
}

void Thttpd::AcceptNewClient(int listen_fd, int epoll_fd) {
  sockaddr_storage remote_addr;
  socklen_t remote_addr_len = sizeof(remote_addr);
  ScopedFd conn_sock(accept4(listen_fd,
                             reinterpret_cast<sockaddr*>(&remote_addr),
                             &remote_addr_len, SOCK_NONBLOCK));
  if (*conn_sock < 0) {
    LOG(ERR) << "accept failed : " << strerror(errno);
    return;
  }
  epoll_event new_event{
      .events = EPOLLIN | EPOLLOUT | EPOLLET,
  };
  new_event.data.fd = *conn_sock;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, *conn_sock, &new_event) < 0) {
    LOG(ERR) << "epoll_ctl on conn_sock failed failed : " << strerror(errno);
    return;
  }

  char addr_str[INET6_ADDRSTRLEN];
  void* in_addr = nullptr;
  if (remote_addr.ss_family == AF_INET) {
    in_addr = &(reinterpret_cast<sockaddr_in*>(&remote_addr)->sin_addr);
  } else {
    in_addr = &(reinterpret_cast<sockaddr_in6*>(&remote_addr)->sin6_addr);
  }
  if (inet_ntop(remote_addr.ss_family, in_addr, addr_str, sizeof(addr_str)) ==
      nullptr) {
    LOG(WARN) << "inet_ntop failed: " << strerror(errno);
  } else {
    addr_str[sizeof(addr_str) - 1] = '\0';
    VLOG(2) << "Connection from: " << addr_str;
  }

  int raw_conn_sock = *conn_sock;

  auto request_handler = std::make_shared<RequestHandler>(
      addr_str, this, thread_pool_.GetNextRunner(), std::move(conn_sock));
  request_handler->Init(request_handler);

  conn_fd_to_handler_.emplace(raw_conn_sock, std::move(request_handler));
}

bool Thttpd::HandleEvents(int event_fd) {
  char buf[BUFSIZ];
  while (true) {
    ssize_t ret = read(event_fd, buf, sizeof(buf));
    if (ret < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        LOG(ERR) << "Read event_fd failed: " << strerror(errno);
      }
      break;
    }
  }

  while (!closed_fds_.Empty()) {
    int fd = closed_fds_.Pop();
    auto it = conn_fd_to_handler_.find(fd);
    if (it == conn_fd_to_handler_.end()) {
      LOG(ERR) << "Unknown socket!";
      continue;
    }

    VLOG(2) << "Disconnected: " << it->second->client_ip();
    conn_fd_to_handler_.erase(it);
  }
  return false;
}

void Thttpd::HandleClient(int fd, uint32_t epoll_events) {
  auto it = conn_fd_to_handler_.find(fd);
  if (it == conn_fd_to_handler_.end()) {
    LOG(ERR) << "Unknown socket!";
    return;
  }
  bool can_read = epoll_events & EPOLLIN;
  bool can_write = epoll_events & EPOLLOUT;

  const auto& request_handler = it->second;

  request_handler->task_runner()->PostTask(BindOnce(
      &RequestHandler::HandleUpdate, request_handler, can_read, can_write));
}
