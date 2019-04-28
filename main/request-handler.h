#ifndef MAIN_REQUEST_HANDLER_
#define MAIN_REQUEST_HANDLER_

#include <string>

#include "absl/types/optional.h"

#include "base/task-runner.h"
#include "main/file-reader.h"
#include "main/request-parser.h"

class Thttpd;

class RequestHandler {
 public:
  RequestHandler(Thttpd* thttpd, TaskRunner* task_runner, int fd);
  RequestHandler(const RequestHandler&) = delete;
  RequestHandler operator=(const RequestHandler&) = delete;

  void HandleUpdate(bool can_read, bool can_write);

  TaskRunner* task_runner() const { return task_runner_; }
  int fd() const { return fd_; }

 private:
  Result<void> ReadFile();

  Thttpd* const thttpd_;
  TaskRunner* const task_runner_;
  const int fd_;

  absl::optional<FileReader> current_file_;
  uint8_t file_buf_[BUFSIZ];
  size_t file_buf_offset_ = 0;
  size_t file_buf_bytes_ = 0;

  RequestParser request_parser_;
};

#endif  // MAIN_REQUEST_HANDLER_
