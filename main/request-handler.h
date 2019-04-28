#ifndef MAIN_REQUEST_HANDLER_
#define MAIN_REQUEST_HANDLER_

#include <string>

#include "base/task-runner.h"
#include "main/request-parser.h"

class RequestHandler {
 public:
  RequestHandler(TaskRunner* task_runner, int fd)
      : task_runner_(task_runner), fd_(fd) {}
  RequestHandler(const RequestHandler&) = delete;
  RequestHandler operator=(const RequestHandler&) = delete;

  void HandleUpdate(bool can_read, bool can_write);

  TaskRunner* task_runner() const { return task_runner_; }
  int fd() const { return fd_; }

 private:
  void CloseSocket();

  TaskRunner* const task_runner_;
  const int fd_;

  RequestParser request_parser_;
};

#endif  // MAIN_REQUEST_HANDLER_
