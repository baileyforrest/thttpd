#ifndef ONCE_CALLBACK_H_
#define ONCE_CALLBACK_H_

#include <memory>
#include <utility>

#include "absl/base/macros.h"
#include "base/once-callback-internal.h"

class OnceCallback {
 public:
  using BoundFunctionPtr =
      std::unique_ptr<once_callback_internal::BoundFunction>;
  OnceCallback() = default;
  explicit OnceCallback(BoundFunctionPtr bound_function)
      : bound_function_(std::move(bound_function)) {}

  OnceCallback(OnceCallback&& other) = default;
  OnceCallback& operator=(OnceCallback&& other) = default;

  void operator()() {
    ABSL_ASSERT(bound_function_);
    bound_function_->Run();
    bound_function_.reset();
  }

 private:
  BoundFunctionPtr bound_function_;
};

template <class F, class... Args>
OnceCallback BindOnce(F&& func, Args&&... args) {
  OnceCallback::BoundFunctionPtr bound_function(
      new once_callback_internal::BoundFunctionImpl<F, Args...>(
          std::forward<F>(func), std::forward<Args>(args)...));
  return OnceCallback(std::move(bound_function));
}

#endif  // ONCE_CALLBACK_H_
