#ifndef BASE_TASK_RUNNER_H_
#define BASE_TASK_RUNNER_H_

#include <atomic>
#include <memory>
#include <thread>

#include "base/mpsc-queue.h"

// A basic TaskRunner to run tasks asynchronously in order.
class TaskRunner {
 public:
  using Task = std::function<void()>;

  // Returns NULL if not running in a TaskRunner.
  static std::shared_ptr<TaskRunner> CurrentTaskRunner() {
    return current_task_runner_;
  }

  static std::shared_ptr<TaskRunner> Create();

  TaskRunner(const TaskRunner&) = delete;
  TaskRunner& operator=(const TaskRunner&) = delete;

  // Calls Stop() if not stopped.
  ~TaskRunner();

  // Stops the TaskRunner. |PostTask| can't be called after stopped. Blocks
  // until all pending tasks are complete.
  void Stop();

  void PostTask(Task task);

  bool IsCurrentThread();

 private:
  static thread_local std::shared_ptr<TaskRunner> current_task_runner_;

  TaskRunner();
  void RunLoop();

  // The thread tasks run on.
  std::thread thread_;

  // True if the task runner is still running.
  std::atomic<bool> running_{true};

  MpscQueue<Task> tasks_;
};

#endif  // BASE_TASK_RUNNER_H_
