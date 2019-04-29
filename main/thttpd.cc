#include "main/thttpd.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstdlib>

#include "absl/base/macros.h"
#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "absl/types/span.h"
#include "base/logging.h"

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
    : config_(config), thread_pool_(config.num_worker_threads) {}

Result<void> Thttpd::Start() {
  int listen_fd = socket(PF_INET6, SOCK_STREAM | SOCK_NONBLOCK, /*protocol=*/0);
  if (listen_fd < 0) {
    return BuildPosixErr("socket failed");
  }

  {
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
      return BuildPosixErr("socksockopt failed");
    }
  }

  sockaddr_in6 addr = {
      .sin6_family = AF_INET6, .sin6_port = htons(config_.port),
  };
  addr.sin6_addr = in6addr_any;
  if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    return BuildPosixErr("bind failed");
  }

  if (listen(listen_fd, kListenBacklog) < 0) {
    return BuildPosixErr("listen failed");
  }

  LOG(INFO) << "Listening on port " << config_.port;

  int epoll_fd = epoll_create1(/*flags=*/0);
  if (epoll_fd < 0) {
    return BuildPosixErr("epoll_create1 failed");
  }

  epoll_event event{
      .events = EPOLLIN,
  };
  event.data.fd = listen_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
    return BuildPosixErr("epoll_ctl on listen_fd failed");
  }

  const auto events = absl::make_unique<std::array<epoll_event, kMaxEvents>>();

  while (true) {
    int num_fds =
        epoll_wait(epoll_fd, events->data(), events->size(), /*timeout=*/-1);
    if (num_fds < 0) {
      return BuildPosixErr("epoll_wait failed");
    }

    for (auto& event : absl::MakeSpan(events->data(), num_fds)) {
      int fd = event.data.fd;
      if (fd == listen_fd) {
        AcceptNewClient(listen_fd, epoll_fd);
        continue;
      }

      auto it = conn_fd_to_handler_.find(fd);
      if (it == conn_fd_to_handler_.end()) {
        close(fd);
        LOG(ERR) << "Unknown socket!";
        continue;
      }
      std::shared_ptr<RequestHandler>& request_handler = it->second;

      bool can_read = event.events & EPOLLIN;
      bool can_write = event.events & EPOLLOUT;

      // Check if the socket was closed.
      // TODO(bcf): This should be handled inside of RequestHandler, which
      // should send an event back to here.
      if (can_read) {
        char byte;
        ssize_t avail = recv(fd, &byte, sizeof(byte), MSG_PEEK);
        if (avail == 0) {
          VLOG(2) << "Disconnected: " << request_handler->client_ip();
          conn_fd_to_handler_.erase(it);
          close(fd);
          continue;
        }
      }

      request_handler->task_runner()->PostTask(
          [request_handler, can_read, can_write] {
            request_handler->HandleUpdate(can_read, can_write);
          });
    }
  }

  return {};
}

void Thttpd::AcceptNewClient(int listen_fd, int epoll_fd) {
  sockaddr_storage remote_addr;
  socklen_t remote_addr_len = sizeof(remote_addr);
  int conn_sock = accept4(listen_fd, reinterpret_cast<sockaddr*>(&remote_addr),
                          &remote_addr_len, SOCK_NONBLOCK);
  if (conn_sock < 0) {
    LOG(ERR) << "accept failed : " << strerror(errno);
    return;
  }
  epoll_event new_event{
      .events = EPOLLIN | EPOLLOUT | EPOLLET,
  };
  new_event.data.fd = conn_sock;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &new_event) < 0) {
    close(conn_sock);
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

  conn_fd_to_handler_.emplace(
      conn_sock, std::make_shared<RequestHandler>(
                     addr_str, this, thread_pool_.GetNextRunner(), conn_sock));
}
