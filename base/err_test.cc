#include "base/err.h"

#include <cerrno>

#include "gtest/gtest.h"

TEST(ErrTest, BuildPosixErr) {
  errno = EPERM;
  Err err = BuildPosixErr("Foo failed");
  EXPECT_EQ(err.msg(), absl::StrCat("Foo failed: ", strerror(EPERM)));
}

TEST(ErrTest, Result) {
  Result<int> result(5);
  ASSERT_TRUE(result.ok());
  EXPECT_EQ(5, *result);

  Result<int> result2(Err("foo"));
  ASSERT_FALSE(result2.ok());
  EXPECT_EQ(result2.err().msg(), "foo");
}

TEST(ErrTest, VoidResult) {
  Result<void> result({});
  EXPECT_TRUE(result.ok());

  Result<void> result2(Err("foo"));
  ASSERT_FALSE(result2.ok());
  EXPECT_EQ(result2.err().msg(), "foo");
}

TEST(ErrTest, Try) {
  auto int_maker = [](bool succeed) -> Result<int> {
    if (succeed) {
      return 5;
    }

    return Err("fail");
  };

  auto string_maker = [&](bool succeed) -> Result<std::string> {
    int int_val = TRY(int_maker(succeed));
    return absl::StrCat(int_val);
  };

  Result<std::string> res = string_maker(true);
  ASSERT_TRUE(res.ok());
  EXPECT_EQ(*res, "5");

  res = string_maker(false);
  ASSERT_FALSE(res.ok());
  EXPECT_EQ(res.err().msg(), "fail");
}
