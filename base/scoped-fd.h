#ifndef BASE_SCOPED_FD_H_
#define BASE_SCOPED_FD_H_

#include <unistd.h>

#include <utility>

class ScopedFd {
 public:
  explicit ScopedFd(int fd = -1) : fd_(fd) {}
  ~ScopedFd() { reset(); }

  ScopedFd(ScopedFd&& other) { *this = std::move(other); }
  ScopedFd& operator=(ScopedFd&& other) {
    reset();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  explicit operator bool() const { return fd_ >= 0; }
  int operator*() const { return fd_; }
  void reset() {
    if (fd_ >= 0) {
      close(fd_);
    }
    fd_ = -1;
  }

 private:
  int fd_ = -1;
};

#endif  // BASE_SCOPED_FD_H_
