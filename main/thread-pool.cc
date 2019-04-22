#include "main/thread-pool.h"

#include <utility>

#include "absl/memory/memory.h"

namespace {

std::vector<std::unique_ptr<TaskRunner>> MakeTaskRunners(size_t num) {
  std::vector<std::unique_ptr<TaskRunner>> result;
  result.reserve(num);

  for (size_t i = 0; i < num; ++i) {
    result.push_back(absl::make_unique<TaskRunner>());
  }

  return result;
}

}  // namespace

ThreadPool::ThreadPool(size_t size) : task_runners_(MakeTaskRunners(size)) {}

void ThreadPool::PostTask(Task task) {
  uint64_t idx = next_task_runner_.fetch_add(1, std::memory_order_acq_rel);
  idx %= task_runners_.size();
  task_runners_[idx]->PostTask(std::move(task));
}
