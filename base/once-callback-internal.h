#ifndef ONCE_CALLBACK_INTERNAL_H_
#define ONCE_CALLBACK_INTERNAL_H_

#include <functional>
#include <tuple>
#include <utility>

namespace once_callback_internal {

class BoundFunction {
 public:
  virtual ~BoundFunction() = default;
  virtual void Run() = 0;
};

template <typename... T>
using tuple_no_ref = std::tuple<typename std::remove_reference<T>::type...>;

template <class F, class... Args>
class BoundFunctionImpl : public BoundFunction {
 public:
  BoundFunctionImpl(F&& func, Args&&... args)
      : func_(std::forward<F>(func)), args_(std::forward<Args>(args)...) {}
  ~BoundFunctionImpl() override = default;

  void Run() override { CallFunc(std::index_sequence_for<Args...>{}); };

 private:
  template <class T = void, size_t... I>
  typename std::enable_if<!std::is_member_function_pointer<F>::value, T>::type
      CallFunc(std::index_sequence<I...>) {
    func_(std::move(std::get<I>(args_))...);
  }

  template <class T = void, size_t... I>
  typename std::enable_if<std::is_member_function_pointer<F>::value, T>::type
      CallFunc(std::index_sequence<I...>) {
    std::mem_fn(func_)(std::move(std::get<I>(args_))...);
  }

  F func_;
  tuple_no_ref<Args...> args_;
};

}  // namespace once_callback_internal

#endif  // ONCE_CALLBACK_INTERNAL_H_
