#ifndef BASE_SCOPED_DESTRUCTOR_H_
#define BASE_SCOPED_DESTRUCTOR_H_

#include <functional>
#include <utility>

#include "absl/base/macros.h"

class ScopedDestructor {
 public:
  explicit ScopedDestructor(std::function<void()> on_destroy)
      : on_destroy_(std::move(on_destroy)) {
    ABSL_ASSERT(on_destroy_);
  }
  ScopedDestructor(ScopedDestructor&&) = default;
  ~ScopedDestructor() {
    if (on_destroy_) {
      on_destroy_();
    }
  }

 private:
  std::function<void()> on_destroy_;
};

#endif  // BASE_SCOPED_DESTRUCTOR_H_
