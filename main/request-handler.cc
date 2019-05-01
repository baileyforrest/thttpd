#include "main/request-handler.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <utility>

#include "base/logging.h"
#include "base/util.h"
#include "main/content-type.h"
#include "main/http-response.h"
#include "main/thttpd.h"

namespace {

constexpr char kIndexHtml[] = "/index.html";
constexpr char kMsgTerminater[] = "\r\n";

}  // namespace

RequestHandler::RequestHandler(absl::string_view client_ip, Thttpd* thttpd,
                               TaskRunner* task_runner, ScopedFd fd)
    : client_ip_(client_ip),
      thttpd_(thttpd),
      task_runner_(task_runner),
      fd_(std::move(fd)) {}

void RequestHandler::HandleUpdate(bool can_read, bool can_write) {
  if (socket_closed_) {
    return;
  }
  while (true) {
    State old_state = state_;
    switch (state_) {
      case State::kPendingRequest:
        state_ = HandlePendingRequest(can_read);
        break;
      case State::kSendingResponseHeader:
        state_ = HandleSendingResponseHeader(can_write);
        break;
      case State::kSendingResponseBody:
        state_ = HandleSendingResponseBody(can_write);
        break;
    }
    if (state_ == old_state) {
      break;
    }
  }
}

Result<bool> RequestHandler::WriteBytes(const char* source) {
  ssize_t remain_bytes = tx_buf_bytes_ - tx_buf_offset_;
  ssize_t sent = send(*fd_, source + tx_buf_offset_, remain_bytes,
                      MSG_NOSIGNAL | MSG_DONTWAIT);
  if (sent < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }

    return BuildPosixErr("send failed");
  }
  tx_buf_offset_ += sent;
  return sent == remain_bytes;
}

RequestHandler::State RequestHandler::HandlePendingRequest(bool can_read) {
  if (!can_read) {
    return state_;
  }
  while (true) {
    char buf[BUFSIZ];
    ssize_t ret = recv(*fd_, buf, sizeof(buf) - 1, /*flags=*/0);
    bool failed = ret < 0;
    if (failed) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return State::kPendingRequest;
      }
      LOG(ERR) << "recv failed: " << strerror(errno);
    }

    // Socket was closed.
    if (failed || ret == 0) {
      socket_closed_ = true;
      thttpd_->NotifySocketClosed(*fd_);
      return State::kPendingRequest;
    }

    auto state = request_parser_.AddData({buf, static_cast<size_t>(ret)});
    switch (state) {
      case RequestParser::State::kInvalid:
        // TODO(bcf): Send 400 error.
        VLOG(1) << "Parsing request failed";
        return State::kPendingRequest;
      case RequestParser::State::kPending:
        continue;
      case RequestParser::State::kReady:
        break;
    }

    HttpRequest request = request_parser_.GetRequestAndReset();
    VLOG(2) << "Got request:\n" << request;
    if (request.method != HttpRequest::Method::kGet) {
      VLOG(1) << "Unsupported method: " << request.method;
      // TODO(bcf): Send 400 error.
      return State::kPendingRequest;
    }

    const std::string& served_path = thttpd_->config().path_to_serve;

    std::string request_file_path = served_path + request.target;
    auto path_or = util::CanonicalizePath(request_file_path);
    if (!path_or.ok()) {
      VLOG(1) << path_or.err();
      // TODO(bcf): Send 400 error.
      return State::kPendingRequest;
    }
    request_file_path = std::move(*path_or);
    if (request_file_path.compare(0, served_path.size(), served_path) != 0) {
      VLOG(1) << "Requested file not inside of path_to_serve: "
              << request_file_path;
      // TODO(bcf): Send 400 error.
      return State::kPendingRequest;
    }

    struct stat stat_buf;
    if (stat(request_file_path.c_str(), &stat_buf) < 0) {
      VLOG(1) << "Failed to stat " << request_file_path << ": "
              << strerror(errno);
      // TODO(bcf): Send 404 error.
      return State::kPendingRequest;
    }

    if (S_ISDIR(stat_buf.st_mode)) {
      request_file_path += kIndexHtml;
    }

    auto file_or = FileReader::Create(request_file_path);
    if (!file_or.ok()) {
      VLOG(1) << "Requested file not found: " << request_file_path;
      // TODO(bcf): Send 400 error.
      return State::kPendingRequest;
    }

    std::map<std::string, std::string> headers = {
        {"Content-Length", absl::StrCat(file_or->size())},
        {"Content-Type",
         std::string(ContentType::ForFilename(request_file_path))},
    };

    auto response =
        HttpResponse::BuildWithDefaultHeaders(HttpResponse::Code::kOk, headers);
    std::ostringstream oss;
    oss << response;

    // Set up variables for next state.
    current_response_header_ = oss.str();
    current_file_ = std::move(*file_or);
    tx_buf_offset_ = 0;
    tx_buf_bytes_ = current_response_header_.size();

    return State::kSendingResponseHeader;
  }

  // Shouldn't reach here.
  ABSL_ASSERT(false);
  return State::kPendingRequest;
}

RequestHandler::State RequestHandler::HandleSendingResponseHeader(
    bool can_write) {
  if (!can_write) {
    return state_;
  }

  ABSL_ASSERT(tx_buf_bytes_ == current_response_header_.size());
  while (tx_buf_offset_ < tx_buf_bytes_) {
    auto send_result = WriteBytes(current_response_header_.data());
    if (!send_result.ok()) {
      LOG(ERR) << send_result.err();
      // TODO(bcf): Send 500 error.
      return State::kPendingRequest;
    }

    if (!*send_result) {
      return state_;
    }
  }

  // Set up variables for next state.
  tx_buf_offset_ = 0;
  tx_buf_bytes_ = 0;
  return State::kSendingResponseBody;
}

RequestHandler::State RequestHandler::HandleSendingResponseBody(
    bool can_write) {
  if (!can_write) {
    return state_;
  }

  while (!current_file_->eof() || tx_buf_offset_ < tx_buf_bytes_) {
    // Read the next chunk of the file.
    if (tx_buf_offset_ == tx_buf_bytes_) {
      auto num_read =
          current_file_->Read(absl::MakeSpan(tx_buf_, sizeof(tx_buf_)));
      if (!num_read.ok()) {
        VLOG(1) << "Read failed: " << num_read.err();
        // TODO(bcf): Send 500
        return State::kPendingRequest;
      }
      tx_buf_offset_ = 0;
      tx_buf_bytes_ = *num_read;
    }

    auto send_result = WriteBytes(tx_buf_);
    if (!send_result.ok()) {
      LOG(ERR) << send_result.err();
      // TODO(bcf): Send 500 error.
      return State::kPendingRequest;
    }

    if (!*send_result) {
      return state_;
    }
  }

  // Clear state and get ready for next request.
  current_response_header_.clear();
  current_file_.reset();
  tx_buf_offset_ = 0;
  tx_buf_bytes_ = 0;
  return State::kPendingRequest;
}
