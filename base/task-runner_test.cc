#include <atomic>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "base/task-runner.h"
#include "gtest/gtest.h"

namespace {

template <typename T>
struct LockedVec {
  void PushBack(T t) {
    absl::WriterMutexLock lock(&mtx_);
    vec.push_back(std::move(t));
  }

  std::vector<T> vec;
  absl::Mutex mtx_;
};

}  // namespace

TEST(TaskRunnerTest, Basic) {
  constexpr int kIters = 100000;
  LockedVec<int> vec;
  auto tr = TaskRunner::Create();

  for (int i = 0; i < kIters; ++i) {
    tr->PostTask(BindOnce([&vec, i] { vec.PushBack(i); }));
  }
  tr->Stop();
  ASSERT_EQ(kIters, vec.vec.size());

  for (int i = 0; i < kIters; ++i) {
    EXPECT_EQ(vec.vec[i], i);
  }
}

TEST(TaskRunnerTest, MultiProducers) {
  constexpr int kIters = 50000;
  constexpr int kNumThreads = 10;

  std::atomic<int> cur_val{0};

  LockedVec<int> vec;
  auto tr = TaskRunner::Create();

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&] {
      while (true) {
        int my_value = cur_val++;
        if (my_value >= kIters) {
          break;
        }
        tr->PostTask(BindOnce([&vec, my_value] { vec.PushBack(my_value); }));
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  tr->Stop();
  std::set<int> got_values(vec.vec.begin(), vec.vec.end());

  for (int i = 0; i < kIters; ++i) {
    EXPECT_EQ(got_values.count(i), 1ul);
  }
}
