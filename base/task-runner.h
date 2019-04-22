#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

// A basic TaskRunner to run tasks asynchronously in order.
// TODO(bcf): Consider using lock free queue.
class TaskRunner {
 public:
  using Task = std::function<void()>;
  using Duration = std::chrono::duration<int64_t>;

  TaskRunner();
  TaskRunner(const TaskRunner&) = delete;
  TaskRunner& operator=(const TaskRunner&) = delete;

  // The destructor blocks until the current task running is complete. All other
  // tasks are dropped.
  ~TaskRunner();

  void PostTask(Task task);
  void PostDelayedTask(Task task, const Duration& delay);
  void WaitUntilIdle();

 private:
  using SteadyTimePoint = std::chrono::time_point<std::chrono::steady_clock>;

  void RunLoop();

  // The thread tasks run on.
  std::thread thread_;

  // Mutex for following members:
  std::mutex mutex_;

  // Used to notify to RunLoop a new task is available.
  std::condition_variable has_task_cv_;

  // Used to notify threads blocked on WaitUntilIdle.
  std::condition_variable is_idle_cv_;

  // True if the task runner is still running.
  bool running_ = true;

  std::queue<Task> tasks_;
  std::deque<std::pair<SteadyTimePoint, Task>> delayed_tasks_;
};
