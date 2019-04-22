#include "main/thread-pool.h"

#include "gtest/gtest.h"

TEST(ThreadPoolTest, Basic) { ThreadPool pool(4); }
