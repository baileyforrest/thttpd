#ifndef MAIN_THREAD_POOL_
#define MAIN_THREAD_POOL_

#include <atomic>
#include <cstdlib>
#include <memory>
#include <vector>

#include "base/task-runner.h"

class ThreadPool {
 public:
  explicit ThreadPool(size_t size);

  // Thread safe.
  void PostTask(OnceCallback task);

  TaskRunner* GetNextRunner();

 private:
  const std::vector<std::shared_ptr<TaskRunner>> task_runners_;
  std::atomic<uint64_t> next_task_runner_{0};
};

#endif  // MAIN_THREAD_POOL_
