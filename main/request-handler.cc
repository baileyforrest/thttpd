#include "main/request-handler.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/logging.h"

namespace {

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

void RequestHandler::HandleUpdate(bool can_read, bool can_write) {
  if (can_read) {
    while (true) {
      char buf[BUFSIZ];
      ssize_t ret = recv(fd_, buf, sizeof(buf) - 1, /*flags=*/0);
      if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          break;
        }
        LOG(ERR) << "recv failed: " << strerror(errno);
        return;
      }
      // Socket close logic should be handled before receiving the request.
      if (ret == 0) {
        LOG(ERR) << "Unexpected empty read";
        return;
      }

      auto state = request_parser_.AddData({buf, static_cast<size_t>(ret)});
      switch (state) {
        case RequestParser::State::kInvalid:
          VLOG(1) << "Parsing request failed";
          break;
        case RequestParser::State::kReady: {
          HttpRequest request = request_parser_.GetRequestAndReset();
          LOG(INFO) << "Got request:\n" << request;
        } break;
        default:
          break;
      }
    }
  }

  if (can_write) {
    if (send(fd_, kResponse, strlen(kResponse), MSG_NOSIGNAL | MSG_DONTWAIT) <
        0) {
      LOG(ERR) << "send failed: " << strerror(errno);
      return;
    }
  }
}
