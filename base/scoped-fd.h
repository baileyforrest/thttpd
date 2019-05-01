#ifndef BASE_SCOPED_FD_H_
#define BASE_SCOPED_FD_H_

#include <unistd.h>

#include <utility>

class ScopedFd {
 public:
  explicit ScopedFd(int fd = -1) : fd_(fd) {}
  ~ScopedFd() { CloseIfSet(); }

  ScopedFd(ScopedFd&& other) { *this = std::move(other); }
  ScopedFd& operator=(ScopedFd&& other) {
    CloseIfSet();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  int get() { return fd_; }

 private:
  void CloseIfSet() {
    if (fd_ >= 0) {
      close(fd_);
    }
  }

  int fd_ = -1;
};

#endif  // BASE_SCOPED_FD_H_
