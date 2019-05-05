#include <chrono>
#include <set>
#include <thread>
#include <vector>

#include "base/mpsc-queue.h"
#include "gtest/gtest.h"

TEST(MpscQueueTest, Basic) {
  MpscQueue<int> queue;
  EXPECT_TRUE(queue.Empty());

  queue.Push(5);
  EXPECT_FALSE(queue.Empty());
  EXPECT_EQ(5, queue.Pop());
  EXPECT_TRUE(queue.Empty());
}

TEST(MpscQueueTest, MultiInsert) {
  constexpr int kMax = 100;

  MpscQueue<int> queue;
  for (int i = 0; i < kMax; ++i) {
    queue.Push(i);
  }

  for (int i = 0; i < kMax; ++i) {
    EXPECT_FALSE(queue.Empty());
    EXPECT_EQ(i, queue.Pop());
  }
  EXPECT_TRUE(queue.Empty());
}

TEST(MpscQueueTest, Mpsc) {
  constexpr int kMax = 100000;
  constexpr int kNumThreads = 10;

  std::atomic<int> cur_value{0};
  std::vector<std::thread> threads;
  MpscQueue<int> queue;
  std::set<int> received;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([&] {
      while (cur_value < kMax) {
        int my_value = cur_value++;
        queue.Push(my_value);
      }
    });
  }

  while (received.size() < kMax) {
    if (!queue.Empty()) {
      EXPECT_TRUE(received.insert(queue.Pop()).second);
    }
  }

  for (int i = 0; i < kMax; ++i) {
    EXPECT_EQ(1, received.count(i));
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

TEST(MpscQueueTest, WaitNotEmpty) {
  constexpr int kMax = 1000;
  MpscQueue<int> queue;
  std::thread thread([&] {
    for (int i = 0; i < kMax; ++i) {
      queue.Push(i);

      // Wait sometimes to introduce randomness to the timing.
      if (i % 4 == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  });

  for (int i = 0; i < kMax; ++i) {
    EXPECT_TRUE(queue.WaitNotEmpty());
    EXPECT_EQ(i, queue.Pop());
  }

  thread.join();
}
