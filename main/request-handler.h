#ifndef MAIN_REQUEST_HANDLER_
#define MAIN_REQUEST_HANDLER_

#include <map>
#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

#include "base/file-reader.h"
#include "base/scoped-fd.h"
#include "base/task-runner.h"
#include "main/compression-cache.h"
#include "main/request-parser.h"

class Thttpd;

class RequestHandler {
 public:
  RequestHandler(absl::string_view client_ip, Thttpd* thttpd,
                 TaskRunner* task_runner, ScopedFd fd);
  RequestHandler(const RequestHandler&) = delete;
  RequestHandler operator=(const RequestHandler&) = delete;

  void Init(std::shared_ptr<RequestHandler> shared_this);

  void HandleUpdate(bool can_read, bool can_write);

  const std::string client_ip() const { return client_ip_; }
  TaskRunner* task_runner() const { return task_runner_; }
  int fd() const { return *fd_; }

 private:
  enum class State {
    kPendingRequest,
    kOpeningCompressedStream,
    kStreamOpened,
    kSendingResponseHeader,
    kSendingResponseBody,
    kSocketClosed,
  };
  void Run();

  // Attempt to write bytes to |fd_| from |source|. Assumes that
  // |tx_buf_offset_| is the offset into |source| to write from and
  // |tx_buf_bytes_| is the total number of bytes in |source|.
  // Returns true if all bytes were written.
  // Returns an error if an error happened.
  Result<bool> WriteBytes(const char* source);

  State HandlePendingRequest();
  void OnCompressedFileRead(Result<CompressionCache::File> file);
  State HandleStreamOpened();
  State HandleSendingResponseHeader();
  State HandleSendingResponseBody();

  const std::string client_ip_;
  Thttpd* const thttpd_;
  TaskRunner* const task_runner_;
  const ScopedFd fd_;
  std::shared_ptr<RequestHandler> shared_this_;

  State state_ = State::kPendingRequest;
  bool can_read_ = false;
  bool can_write_ = false;

  std::map<std::string, std::string> response_header_fields_;
  std::string response_header_string_;

  std::unique_ptr<Reader> reader_;
  char tx_buf_[BUFSIZ];
  size_t tx_buf_offset_ = 0;
  size_t tx_buf_bytes_ = 0;

  RequestParser request_parser_;
};

#endif  // MAIN_REQUEST_HANDLER_
