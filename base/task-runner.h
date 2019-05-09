#ifndef BASE_TASK_RUNNER_H_
#define BASE_TASK_RUNNER_H_

#include <atomic>
#include <memory>
#include <thread>

#include "base/mpsc-queue.h"
#include "base/once-callback.h"

// A basic TaskRunner to run tasks asynchronously in order.
class TaskRunner {
 public:
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

  void PostTask(OnceCallback task);

  bool IsCurrentThread();

 private:
  static thread_local std::shared_ptr<TaskRunner> current_task_runner_;

  void Init(std::shared_ptr<TaskRunner> shared_this);

  TaskRunner();
  void RunLoop();

  // The thread tasks run on.
  std::thread thread_;

  // True if the task runner is still running.
  std::atomic<bool> running_{true};

  MpscQueue<OnceCallback> tasks_;
};

#endif  // BASE_TASK_RUNNER_H_
