#include "main/request-handler.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>

#include "base/logging.h"
#include "base/util.h"
#include "main/thttpd.h"

namespace {

constexpr char kMsgTerminater[] = "\r\n";

}  // namespace

RequestHandler::RequestHandler(Thttpd* thttpd, TaskRunner* task_runner, int fd)
    : thttpd_(thttpd), task_runner_(task_runner), fd_(fd) {}

void RequestHandler::HandleUpdate(bool can_read, bool can_write) {
  // If we don't have a file open, we're still waiting for the request.
  if (!current_file_ && can_read) {
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
          // TODO(bcf): Send 400 error.
          VLOG(1) << "Parsing request failed";
          break;
        case RequestParser::State::kReady: {
          HttpRequest request = request_parser_.GetRequestAndReset();
          VLOG(2) << "Got request:\n" << request;
          if (request.method != HttpRequest::Method::kGet) {
            VLOG(1) << "Unsupported method: " << request.method;
            // TODO(bcf): Send 400 error.
            break;
          }

          const std::string& served_path = thttpd_->config().path_to_serve;

          std::string request_file_path = served_path + request.target;
          auto path_or = util::CanonicalizePath(request_file_path);
          if (!path_or.ok()) {
            VLOG(1) << path_or.err();
            // TODO(bcf): Send 400 error.
            break;
          }
          request_file_path = std::move(*path_or);
          if (request_file_path.compare(0, served_path.size(), served_path) !=
              0) {
            VLOG(1) << "Requested file not inside of path_to_serve: "
                    << request_file_path;
            // TODO(bcf): Send 400 error.
            break;
          }
          auto file_or = FileReader::Create(request_file_path);
          if (!file_or.ok()) {
            VLOG(1) << "Requested file not found: " << request_file_path;
            // TODO(bcf): Send 404 error.
            break;
          }
          current_file_ = std::move(*file_or);
          auto read_result = ReadFile();
          if (!read_result.ok()) {
            VLOG(1) << read_result.err();
            // TODO(bcf): Send 500 error.
          }
        } break;
        default:
          break;
      }
    }
  }

  if (current_file_ && can_write) {
    // TODO(bcf): Send header and footer.
    while (true) {
      // Check if we're done sending the file.
      if (current_file_->eof() && file_buf_offset_ == file_buf_bytes_) {
        current_file_.reset();
        return;
      }
      size_t remain_bytes = file_buf_bytes_ - file_buf_offset_;
      LOG(ERR) << "Sending " << remain_bytes;
      ssize_t sent = send(fd_, file_buf_ + file_buf_offset_, remain_bytes,
                          MSG_NOSIGNAL | MSG_DONTWAIT);
      if (sent < 0) {
        LOG(ERR) << "send failed: " << strerror(errno);
        return;
      }
      file_buf_offset_ += sent;

      if (file_buf_offset_ == file_buf_bytes_) {
        if (!current_file_->eof()) {
          auto read_result = ReadFile();
          if (!read_result.ok()) {
            VLOG(1) << read_result.err();
            // TODO(bcf): Send 500 error.
          }
        }
      }
    }
  }
}

Result<void> RequestHandler::ReadFile() {
  ABSL_ASSERT(current_file_);
  ABSL_ASSERT(file_buf_offset_ == file_buf_bytes_);
  auto num_read =
      current_file_->Read(absl::MakeSpan(file_buf_, sizeof(file_buf_)));
  if (!num_read.ok()) {
    VLOG(1) << "Read failed: " << num_read.err();
    return num_read.err();
  }
  file_buf_offset_ = 0;
  file_buf_bytes_ = *num_read;

  return {};
}
