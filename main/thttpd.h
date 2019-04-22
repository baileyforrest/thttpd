#ifndef MAIN_THTTPD_H_
#define MAIN_THTTPD_H_

#include <memory>

#include "absl/base/attributes.h"
#include "base/err.h"

class Thttpd {
 public:
  Thttpd(const Thttpd&) = delete;
  Thttpd& operator=(const Thttpd&) = delete;

  static Result<std::unique_ptr<Thttpd>> Create(int port);

  // Blocks.
  ABSL_MUST_USE_RESULT Result<void> Start();

 private:
  explicit Thttpd(int port);

  const int port_;
};

#endif  // MAIN_THTTPD_H_
