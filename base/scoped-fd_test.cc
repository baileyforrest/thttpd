#include <fcntl.h>
#include <unistd.h>

#include <memory>

#include "absl/memory/memory.h"
#include "base/scoped-fd.h"
#include "gtest/gtest.h"

namespace {

bool IsValidFd(int fd) { return fcntl(fd, F_GETFD) != -1 || errno != EBADF; }

int CreateFd() { return open("/dev/null", /*flags=*/0); }

}  // namespace

TEST(ScopedFdTest, Destructor) {
  int fd = CreateFd();
  ASSERT_TRUE(IsValidFd(fd));
  {
    ScopedFd sfd(fd);
    EXPECT_EQ(fd, sfd.get());
  }
  ASSERT_FALSE(IsValidFd(fd));
}

TEST(ScopedFdTest, MoveConstructor) {
  int fd = CreateFd();
  ASSERT_TRUE(IsValidFd(fd));
  std::unique_ptr<ScopedFd> outer;
  {
    ScopedFd sfd(fd);
    EXPECT_EQ(fd, sfd.get());
    outer = absl::make_unique<ScopedFd>(std::move(sfd));
    EXPECT_EQ(-1, sfd.get());
  }
  ASSERT_TRUE(IsValidFd(fd));
  outer.reset();
  ASSERT_FALSE(IsValidFd(fd));
}

TEST(ScopedFdTest, MoveAssign) {
  int fd1 = CreateFd();
  ASSERT_TRUE(IsValidFd(fd1));

  int fd2 = CreateFd();
  ASSERT_TRUE(IsValidFd(fd2));

  auto outer = absl::make_unique<ScopedFd>(fd1);

  {
    ScopedFd sfd(fd2);
    EXPECT_EQ(fd2, sfd.get());
    *outer = std::move(sfd);
    EXPECT_EQ(-1, sfd.get());
    EXPECT_FALSE(IsValidFd(fd1));
    EXPECT_TRUE(IsValidFd(fd2));
  }
  outer.reset();
  ASSERT_FALSE(IsValidFd(fd2));
}
