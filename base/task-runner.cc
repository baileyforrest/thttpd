#include "base/task-runner.h"

#include <utility>

// static
thread_local std::shared_ptr<TaskRunner> TaskRunner::current_task_runner_;

// static
std::shared_ptr<TaskRunner> TaskRunner::Create() {
  return std::shared_ptr<TaskRunner>(new TaskRunner);
}

TaskRunner::TaskRunner() {
  thread_ = std::thread([this] { RunLoop(); });
}

TaskRunner::~TaskRunner() {
  if (running_.load(std::memory_order_relaxed)) {
    Stop();
  }
}

void TaskRunner::Stop() {
  running_.store(false, std::memory_order_release);
  PostTask([] {});  // Post empty task to break out of WaitNotEmpty().
  thread_.join();
}

void TaskRunner::PostTask(Task task) { tasks_.Push(std::move(task)); }

bool TaskRunner::IsCurrentThread() {
  return current_task_runner_.get() == this;
}

void TaskRunner::RunLoop() {
  while (running_.load(std::memory_order_acquire)) {
    tasks_.WaitNotEmpty();
    while (!tasks_.Empty()) {
      Task task = tasks_.Pop();
      task();
    }
  }
}
