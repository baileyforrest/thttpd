#ifndef MAIN_REQUEST_HANDLER_
#define MAIN_REQUEST_HANDLER_

#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

#include "base/scoped-fd.h"
#include "base/task-runner.h"
#include "main/file-reader.h"
#include "main/request-parser.h"

class Thttpd;

class RequestHandler {
 public:
  RequestHandler(absl::string_view client_ip, Thttpd* thttpd,
                 TaskRunner* task_runner, ScopedFd fd);
  RequestHandler(const RequestHandler&) = delete;
  RequestHandler operator=(const RequestHandler&) = delete;

  void HandleUpdate(bool can_read, bool can_write);

  const std::string client_ip() const { return client_ip_; }
  TaskRunner* task_runner() const { return task_runner_; }
  int fd() const { return *fd_; }

 private:
  enum class State {
    kPendingRequest,
    kSendingResponseHeader,
    kSendingResponseBody,
  };

  // Attempt to write bytes to |fd_| from |source|. Assumes that
  // |tx_buf_offset_| is the offset into |source| to write from and
  // |tx_buf_bytes_| is the total number of bytes in |source|.
  // Returns number of bytes written.
  // Returns 0 if no more bytes can be written right now.
  // Returns an error if an error happened.
  Result<size_t> WriteBytes(const char* source);

  State HandlePendingRequest(bool can_read);
  State HandleSendingResponseHeader(bool can_write);
  State HandleSendingResponseBody(bool can_write);

  const std::string client_ip_;
  Thttpd* const thttpd_;
  TaskRunner* const task_runner_;
  const ScopedFd fd_;
  bool socket_closed_ = false;

  State state_ = State::kPendingRequest;

  std::string current_response_header_;

  absl::optional<FileReader> current_file_;
  char tx_buf_[BUFSIZ];
  size_t tx_buf_offset_ = 0;
  size_t tx_buf_bytes_ = 0;

  RequestParser request_parser_;
};

#endif  // MAIN_REQUEST_HANDLER_
