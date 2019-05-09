#include "base/task-runner.h"

#include <utility>

// static
thread_local std::shared_ptr<TaskRunner> TaskRunner::current_task_runner_;

// static
std::shared_ptr<TaskRunner> TaskRunner::Create() {
  std::shared_ptr<TaskRunner> ret(new TaskRunner);
  ret->Init(ret);
  return ret;
}

TaskRunner::TaskRunner() = default;

TaskRunner::~TaskRunner() {
  if (running_.load(std::memory_order_relaxed)) {
    Stop();
  }
}

void TaskRunner::Stop() {
  running_.store(false, std::memory_order_release);
  PostTask(BindOnce([] {}));  // Post empty task to break out of WaitNotEmpty().
  thread_.join();
}

void TaskRunner::PostTask(OnceCallback task) { tasks_.Push(std::move(task)); }

bool TaskRunner::IsCurrentThread() {
  return current_task_runner_.get() == this;
}

void TaskRunner::Init(std::shared_ptr<TaskRunner> shared_this) {
  thread_ =
      std::thread([ this, shared_this = std::move(shared_this) ]() mutable {
        current_task_runner_ = std::move(shared_this);
        RunLoop();
      });
}

void TaskRunner::RunLoop() {
  while (running_.load(std::memory_order_acquire)) {
    tasks_.WaitNotEmpty();
    while (!tasks_.Empty()) {
      OnceCallback task = tasks_.Pop();
      task();
    }
  }
}
