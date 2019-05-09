#include "main/request-handler.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

void RequestHandler::Init(std::shared_ptr<RequestHandler> shared_this) {
  shared_this_ = std::move(shared_this);
}

void RequestHandler::HandleUpdate(bool can_read, bool can_write) {
  ABSL_ASSERT(task_runner_->IsCurrentThread());
  can_read_ = can_read;
  can_write_ = can_write;
  Run();
}

void RequestHandler::Run() {
  ABSL_ASSERT(task_runner_->IsCurrentThread());
  State old_state = state_;
  do {
    switch (state_) {
      case State::kPendingRequest:
        state_ = HandlePendingRequest();
        break;
      case State::kOpeningCompressedStream:
        return;
      case State::kStreamOpened:
        state_ = HandleStreamOpened();
      case State::kSendingResponseHeader:
        state_ = HandleSendingResponseHeader();
        break;
      case State::kSendingResponseBody:
        state_ = HandleSendingResponseBody();
        break;
      case State::kSocketClosed:
        return;
    }
  } while (state_ != old_state);
}

Result<bool> RequestHandler::WriteBytes(const char* source) {
  ABSL_ASSERT(task_runner_->IsCurrentThread());
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

RequestHandler::State RequestHandler::HandlePendingRequest() {
  ABSL_ASSERT(task_runner_->IsCurrentThread());
  if (!can_read_) {
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
      shared_this_.reset();
      thttpd_->NotifySocketClosed(*fd_);
      return State::kSocketClosed;
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
      if (stat(request_file_path.c_str(), &stat_buf) < 0) {
        VLOG(1) << "Failed to stat " << request_file_path << ": "
                << strerror(errno);
        // TODO(bcf): Send 404 error.
        return State::kPendingRequest;
      }
    }

    absl::string_view content_type =
        ContentType::ForFilename(request_file_path);

    response_header_fields_.emplace("Content-Type", std::string(content_type));
    auto modified_date = HttpResponse::FormatTime(stat_buf.st_mtime);
    if (!modified_date.ok()) {
      VLOG(1) << "Failed to generate Last-Modified";
    } else {
      response_header_fields_.emplace("Last-Modified",
                                      std::move(*modified_date));
    }

    // TODO(bcf): Enable compression.
    if (false && ContentType::ShouldCompress(content_type)) {
      thttpd_->compression_cache()->RequestFile(
          request_file_path, [self = shared_this_](auto file) {
            self->task_runner_->PostTask(
                BindOnce(&RequestHandler::OnCompressedFileRead, self.get(),
                         std::move(file)));
          });
      return State::kOpeningCompressedStream;
    }

    auto file_or = FileReader::Create(request_file_path);
    if (!file_or.ok()) {
      VLOG(1) << "Requested file not found: " << request_file_path;
      // TODO(bcf): Send 400 error.
      return State::kPendingRequest;
    }

    response_header_fields_.emplace("Content-Length",
                                    absl::StrCat(file_or->size()));

    reader_ = absl::make_unique<FileReader>(std::move(*file_or));

    return State::kStreamOpened;
  }

  // Shouldn't reach here.
  ABSL_ASSERT(false);
  return State::kPendingRequest;
}

void RequestHandler::OnCompressedFileRead(Result<CompressionCache::File> file) {
  ABSL_ASSERT(task_runner_->IsCurrentThread());
  if (file.ok()) {
    response_header_fields_.emplace("Content-Length",
                                    absl::StrCat(file->size()));
    reader_ = absl::make_unique<CompressionCache::File>(std::move(*file));
    state_ = State::kStreamOpened;
  } else {
    // TODO(bcf): Send 400 error.
    state_ = State::kPendingRequest;
  }

  Run();
}

RequestHandler::State RequestHandler::HandleStreamOpened() {
  ABSL_ASSERT(task_runner_->IsCurrentThread());
  ABSL_ASSERT(reader_);
  auto response = HttpResponse::BuildWithDefaultHeaders(
      HttpResponse::Code::kOk, response_header_fields_);
  response_header_fields_.clear();

  std::ostringstream oss;
  oss << response;

  // Set up variables for next state.
  response_header_string_ = oss.str();
  tx_buf_offset_ = 0;
  tx_buf_bytes_ = response_header_string_.size();

  return State::kSendingResponseHeader;
}

RequestHandler::State RequestHandler::HandleSendingResponseHeader() {
  ABSL_ASSERT(task_runner_->IsCurrentThread());
  if (!can_write_) {
    return state_;
  }

  ABSL_ASSERT(tx_buf_bytes_ == response_header_string_.size());
  while (tx_buf_offset_ < tx_buf_bytes_) {
    auto send_result = WriteBytes(response_header_string_.data());
    if (!send_result.ok()) {
      LOG(ERR) << send_result.err();
      // TODO(bcf): Send 500 error.
      return State::kPendingRequest;
    }

    if (!*send_result) {
      return state_;
    }
  }
  response_header_string_.clear();

  // Set up variables for next state.
  tx_buf_offset_ = 0;
  tx_buf_bytes_ = 0;
  return State::kSendingResponseBody;
}

RequestHandler::State RequestHandler::HandleSendingResponseBody() {
  ABSL_ASSERT(task_runner_->IsCurrentThread());
  if (!can_write_) {
    return state_;
  }

  while (true) {
    // Read the next chunk of the file.
    if (tx_buf_offset_ == tx_buf_bytes_) {
      auto num_read = reader_->Read(absl::MakeSpan(tx_buf_, sizeof(tx_buf_)));
      if (!num_read.ok()) {
        VLOG(1) << "Read failed: " << num_read.err();
        // TODO(bcf): Send 500
        return State::kPendingRequest;
      }
      if (*num_read == -1) {  // EOF
        break;
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
  reader_.reset();
  tx_buf_offset_ = 0;
  tx_buf_bytes_ = 0;
  return State::kPendingRequest;
}
