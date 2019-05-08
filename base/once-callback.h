#ifndef ONCE_CALLBACK_H_
#define ONCE_CALLBACK_H_

#include <memory>
#include <utility>

#include "absl/base/macros.h"
#include "base/once-callback-internal.h"

class OnceCallback {
 public:
  template <class F, class... Args>
  OnceCallback(F&& func, Args&&... args)
      : bound_function_(
            new once_callback_internal::BoundFunctionImpl<F, Args...>(
                std::forward<F>(func), std::forward<Args>(args)...)) {}

  void operator()() {
    ABSL_ASSERT(bound_function_);
    bound_function_->Run();
    bound_function_.reset();
  }

 private:
  std::unique_ptr<once_callback_internal::BoundFunction> bound_function_;
};

#endif  // ONCE_CALLBACK_H_
