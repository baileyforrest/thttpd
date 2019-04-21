#include <arpa/inet.h>
#include <err.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "absl/base/macros.h"
#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"

namespace {

constexpr int kListenBacklog = 128;
constexpr int kMaxEvents = 4096;

constexpr char kResponse[] = R"http(HTTP/1.1 200 OK
Accept-Ranges: bytes
Cache-Control: max-age=604800
Content-Type: text/html; charset=UTF-8
Date: Sun, 21 Apr 2019 10:08:17 GMT
Etag: "1541025663+gzip"
Expires: Sun, 28 Apr 2019 10:08:17 GMT
Last-Modified: Fri, 09 Aug 2013 23:54:35 GMT
Server: ECS (sjc/4E8B)
Vary: Accept-Encoding
X-Cache: HIT
Content-Length: 1257

<!doctype html>
<html>
<head>
    <title>Example Domain</title>

    <meta charset="utf-8" />
    <meta http-equiv="Content-type" content="text/html; charset=utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style type="text/css">
    body {
        background-color: #f0f0f2;
        margin: 0;
        padding: 0;
        font-family: "Open Sans", "Helvetica Neue", Helvetica, Arial, sans-serif;
    }
    div {
        width: 600px;
        margin: 5em auto;
        padding: 50px;
        background-color: #fff;
        border-radius: 1em;
    }
    a:link, a:visited {
        color: #38488f;
        text-decoration: none;
    }
    @media (max-width: 700px) {
        body {
            background-color: #fff;
        }
        div {
            width: auto;
            margin: 0 auto;
            border-radius: 0;
            padding: 1em;
        }
    }
    </style>
</head>

<body>
<div>
    <h1>Example Domain</h1>
    <p>This domain is established to be used for illustrative examples in documents. You may use this
    domain in examples without prior coordination or asking for permission.</p>
    <p><a href="http://www.iana.org/domains/example">More information...</a></p>
</div>
</body>
</html>
)http";

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    errx(EXIT_FAILURE, "Need port");
  }

  int port;
  if (!absl::SimpleAtoi(argv[1], &port)) {
    errx(EXIT_FAILURE, "Failed to parse port");
  }

  if (port > std::numeric_limits<uint16_t>::max()) {
    errx(EXIT_FAILURE, "port too big");
  }

  int listen_fd = socket(PF_INET6, SOCK_STREAM | SOCK_NONBLOCK, /*protocol=*/0);
  if (listen_fd < 0) {
    err(EXIT_FAILURE, "socket");
  }

  {
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
      err(EXIT_FAILURE, "socksockopt");
    }
  }

  std::cout << "Listening on port " << port << "\n";

  sockaddr_in6 addr = {
      .sin6_family = AF_INET6, .sin6_port = htons(port),
  };
  addr.sin6_addr = in6addr_any;
  if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    err(EXIT_FAILURE, "bind");
  }

  if (listen(listen_fd, kListenBacklog) < 0) {
    err(EXIT_FAILURE, "listen");
  }

  int epoll_fd = epoll_create1(/*flags=*/0);
  if (epoll_fd < 0) {
    err(EXIT_FAILURE, "epoll_create1");
  }

  epoll_event event{
      .events = EPOLLIN,
  };
  event.data.fd = listen_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
    err(EXIT_FAILURE, "epoll_ctl: listen_sock");
  }

  const auto events = absl::make_unique<std::array<epoll_event, kMaxEvents>>();

  while (true) {
    int num_fds =
        epoll_wait(epoll_fd, events->data(), events->size(), /*timeout=*/-1);
    if (num_fds < 0) {
      err(EXIT_FAILURE, "epoll_wait");
    }

    for (auto& event : absl::MakeSpan(events->data(), num_fds)) {
      int fd = event.data.fd;
      if (fd == listen_fd) {
        sockaddr_storage remote_addr;
        socklen_t remote_addr_len = sizeof(remote_addr);
        int conn_sock =
            accept4(listen_fd, reinterpret_cast<sockaddr*>(&remote_addr),
                    &remote_addr_len, SOCK_NONBLOCK);
        if (conn_sock < 0) {
          err(EXIT_FAILURE, "accept");
        }
        epoll_event new_event{
            .events = EPOLLIN | EPOLLET,
        };
        new_event.data.fd = conn_sock;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &new_event) < 0) {
          err(EXIT_FAILURE, "epoll_ctl: conn_sock");
        }

        char addr_str[INET6_ADDRSTRLEN];
        void* in_addr = nullptr;
        if (remote_addr.ss_family == AF_INET) {
          in_addr = &(reinterpret_cast<sockaddr_in*>(&remote_addr)->sin_addr);
        } else {
          in_addr = &(reinterpret_cast<sockaddr_in6*>(&remote_addr)->sin6_addr);
        }
        if (inet_ntop(remote_addr.ss_family, in_addr, addr_str,
                      sizeof(addr_str)) == nullptr) {
          warn("get_in_addr");
        } else {
          addr_str[sizeof(addr_str) - 1] = '\0';
          std::cout << "Connection from: " << addr_str << "\n";
        }

        continue;
      }

      char buf[BUFSIZ];
      ssize_t ret = recv(fd, buf, sizeof(buf) - 1, /*flags=*/0);
      if (ret < 0) {
        err(EXIT_FAILURE, "recv");
      }
      if (ret == 0) {
        close(fd);
        continue;
      }
      buf[ret] = '\0';
      std::cout << "Got request:\n";
      std::cout << buf << "\n";

      if (send(fd, kResponse, strlen(kResponse), MSG_NOSIGNAL | MSG_DONTWAIT) <
          0) {
        err(EXIT_FAILURE, "send");
      }
      close(fd);
    }
  }

  return EXIT_SUCCESS;
}
