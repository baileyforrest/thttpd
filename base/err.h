#ifndef BASE_ERR_H_
#define BASE_ERR_H_

// Kind of inspired by Rust's error handling.

#include <string>
#include <utility>

#include "absl/base/macros.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

class Err {
 public:
  explicit Err(absl::string_view msg) : msg_(msg) {}
  Err(const Err&) = default;
  Err& operator=(const Err&) = default;
  Err(Err&&) = default;
  Err& operator=(Err&&) = default;

  const std::string& msg() const { return msg_; }

 private:
  std::string msg_;
};

inline Err BuildPosixErr(absl::string_view prefix) {
  auto msg = absl::StrCat(prefix, ": ", strerror(errno));
  return Err(msg);
}

inline std::ostream& operator<<(std::ostream& os, const Err& err) {
  return os << err.msg();
}

template <typename T>
class Result {
 public:
  Result(T result)  // NOLINT(runtime/explicit)
      : result_(std::move(result)),
        state_(State::kOk) {}
  Result(Err error)  // NOLINT(runtime/explicit)
      : err_(std::move(error)),
        state_(State::kErr) {}
  Result(Result&& other) { *this = std::move(other); }
  Result& operator=(Result&& other) {
    Destroy();
    state_ = other.state_;
    switch (state_) {
      case State::kOk:
        new (&result_) T(std::move(other.result_));
        break;
      case State::kErr:
        new (&err_) Err(std::move(other.err_));
        break;
      case State::kDestroyed:
        break;
    }
    return *this;
  }

  ~Result() { Destroy(); }

  bool ok() const { return state_ == State::kOk; }

  T& result() {
    ABSL_ASSERT(state_ == State::kOk);
    return result_;
  }
  T& operator*() { return result(); }
  T* operator->() { return &result(); }

  Err& err() {
    ABSL_ASSERT(state_ == State::kErr);
    return err_;
  }

 private:
  void Destroy() {
    switch (state_) {
      case State::kOk:
        result_.T::~T();
        break;
      case State::kErr:
        err_.Err::~Err();
        break;
      case State::kDestroyed:
        break;
    }
  }

  union {
    T result_;
    Err err_;
  };
  enum class State : uint8_t {
    kOk,
    kErr,
    kDestroyed,
  };

  State state_ = State::kDestroyed;
};

template <>
class Result<void> {
 public:
  struct placeholder {};

  Result() : err_(""), ok_(true) {}
  Result(Err error)  // NOLINT(runtime/explicit)
      : err_(std::move(error)),
        ok_(false) {}
  Result(Result&&) = default;
  Result& operator=(Result&&) = default;
  bool ok() const { return ok_; }

  // Need to return something for the TRY macro.
  placeholder operator*() {
    ABSL_ASSERT(ok_);
    return {};
  }

  Err& err() {
    ABSL_ASSERT(!ok_);
    return err_;
  }

 private:
  Err err_;
  bool ok_ = false;
  bool destroyed_ = false;
};

#define TRY(expr)       \
  ({                    \
    auto res = (expr);  \
    if (!res.ok()) {    \
      return res.err(); \
    }                   \
    std::move(*res);    \
  })

#endif  // BASE_ERR_H_
