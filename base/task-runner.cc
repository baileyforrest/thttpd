#include "base/task-runner.h"

#include <utility>

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

void TaskRunner::RunLoop() {
  while (running_.load(std::memory_order_acquire)) {
    tasks_.WaitNotEmpty();
    while (!tasks_.Empty()) {
      Task task = tasks_.Pop();
      task();
    }
  }
}
