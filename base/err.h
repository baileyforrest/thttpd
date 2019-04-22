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
        ok_(true) {}
  Result(Err error)  // NOLINT(runtime/explicit)
      : err_(std::move(error)),
        ok_(false) {}
  Result(Result&& other) { *this = std::move(other); }
  Result& operator=(Result&& other) {
    Destroy();
    ok_ = other.ok_;
    if (ok_) {
      result_ = std::move(other.result_);
    } else {
      err_ = std::move(other.err_);
    }
    return *this;
  }

  ~Result() { Destroy(); }

  bool ok() const { return ok_; }

  T& result() {
    ABSL_ASSERT(ok_);
    return result_;
  }
  T& operator*() { return result(); }
  T* operator->() { return &result(); }

  Err& err() {
    ABSL_ASSERT(!ok_);
    return err_;
  }

 private:
  void Destroy() {
    if (ok()) {
      result_.T::~T();
    } else {
      err_.Err::~Err();
    }
  }

  union {
    T result_;
    Err err_;
  };
  bool ok_ = false;
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
