#include "base/once-callback.h"

#include <string>

#include "absl/memory/memory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;

namespace {

class MockInterface {
 public:
  MOCK_METHOD1(MethodCopy, void(std::string));
  MOCK_METHOD1(MethodRef, void(const std::string&));
  MOCK_METHOD1(MethodMove, void(std::unique_ptr<std::string> s));
};

}  // namespace

TEST(OnceCallbackTest, BindLambda) {
  bool called = false;
  auto bound = BindOnce([&] { called = true; });
  bound();
  EXPECT_TRUE(called);
}

TEST(OnceCallbackTest, MethodCopy) {
  MockInterface mi;
  std::string s = "foo";
  auto bound = BindOnce(&MockInterface::MethodCopy, &mi, s);
  s.clear();
  EXPECT_CALL(mi, MethodCopy("foo"));
  bound();
}

TEST(OnceCallbackTest, MethodRef) {
  MockInterface mi;
  auto s = absl::make_unique<std::string>("foo");
  auto bound = BindOnce(&MockInterface::MethodRef, &mi, *s);
  s.reset();
  EXPECT_CALL(mi, MethodRef("foo"));
  bound();
}

TEST(OnceCallbackTest, MethodMove) {
  MockInterface mi;
  auto s = absl::make_unique<std::string>("foo");

  auto bound = BindOnce(&MockInterface::MethodMove, &mi, std::move(s));
  s.reset();
  EXPECT_CALL(mi, MethodMove(_))
      .WillOnce([](std::unique_ptr<std::string> s) {
        ASSERT_TRUE(s);
        EXPECT_EQ(*s, "foo");
      });
  bound();
}
