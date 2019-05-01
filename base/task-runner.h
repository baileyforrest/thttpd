#pragma once

#include <atomic>
#include <thread>

#include "base/mpsc-queue.h"

// A basic TaskRunner to run tasks asynchronously in order.
class TaskRunner {
 public:
  using Task = std::function<void()>;

  TaskRunner();
  TaskRunner(const TaskRunner&) = delete;
  TaskRunner& operator=(const TaskRunner&) = delete;

  // Calls Stop() if not stopped.
  ~TaskRunner();

  // Stops the TaskRunner. |PostTask| can't be called after stopped. Blocks
  // until all pending tasks are complete.
  void Stop();

  void PostTask(Task task);

 private:
  void RunLoop();

  // The thread tasks run on.
  std::thread thread_;

  // True if the task runner is still running.
  std::atomic<bool> running_{true};

  MpscQueue<Task> tasks_;
};
