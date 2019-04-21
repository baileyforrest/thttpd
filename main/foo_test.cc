#include "main/foo.h"

#include "gtest/gtest.h"

TEST(FooTest, GetFoo) {
  Foo foo("foobar");
  EXPECT_EQ(foo.get_foo(), "foobar");
}
