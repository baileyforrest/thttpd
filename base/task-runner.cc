#include "base/task-runner.h"

#include <cassert>

TaskRunner::TaskRunner() {
  thread_ = std::thread([this] { RunLoop(); });
}

TaskRunner::~TaskRunner() {
  std::unique_lock<std::mutex> lock(mutex_);
  running_ = false;
  has_task_cv_.notify_one();
  lock.unlock();
  thread_.join();
}

void TaskRunner::PostTask(Task task) {
  std::unique_lock<std::mutex> lock(mutex_);
  tasks_.push(std::move(task));
  has_task_cv_.notify_one();
}

void TaskRunner::PostDelayedTask(Task task, const Duration& delay) {
  auto run_at = std::chrono::steady_clock::now() + delay;

  std::unique_lock<std::mutex> lock(mutex_);
  auto it = delayed_tasks_.begin();
  for (; it != delayed_tasks_.end(); ++it) {
    if (run_at <= it->first) break;
  }

  delayed_tasks_.insert(it, std::make_pair(run_at, std::move(task)));
  has_task_cv_.notify_one();
}

void TaskRunner::WaitUntilIdle() {
  std::unique_lock<std::mutex> lock(mutex_);
  is_idle_cv_.wait(lock,
                   [this] { return tasks_.empty() && delayed_tasks_.empty(); });
}

void TaskRunner::RunLoop() {
  while (true) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (true) {
      if (!running_) return;

      if (!tasks_.empty()) break;

      if (!delayed_tasks_.empty()) {
        auto now = std::chrono::steady_clock::now();
        if (now >= delayed_tasks_.front().first) {
          break;
        }
      }

      if (delayed_tasks_.empty()) {
        has_task_cv_.wait(lock);
      } else {
        has_task_cv_.wait_until(lock, delayed_tasks_.front().first);
      }
    }

    Task task;
    if (!tasks_.empty()) {
      task = std::move(tasks_.front());
      tasks_.pop();
    } else if (!delayed_tasks_.empty()) {
      assert(std::chrono::steady_clock::now() >= delayed_tasks_.front().first);
      task = std::move(delayed_tasks_.front().second);
      delayed_tasks_.pop_front();
    } else {
      assert(false);
    }

    if (tasks_.empty() && delayed_tasks_.empty()) {
      is_idle_cv_.notify_all();
    }

    lock.unlock();
    task();
  }
}
