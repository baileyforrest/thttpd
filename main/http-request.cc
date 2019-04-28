#include <algorithm>
#include <vector>

#include "absl/base/macros.h"
#include "base/logging.h"
#include "main/http-request.h"

namespace {

const char* kMethodStrs[] = {
    "GET", "INVALID",
};
static_assert(ABSL_ARRAYSIZE(kMethodStrs) ==
                  static_cast<size_t>(HttpRequest::Method::kInvalid) + 1,
              "");

}  // namespace

// static
HttpRequest::Method HttpRequest::ParseMethod(absl::string_view text) {
  for (size_t i = 0; i < ABSL_ARRAYSIZE(kMethodStrs); ++i) {
    if (kMethodStrs[i] == text) {
      return static_cast<Method>(i);
    }
  }

  return HttpRequest::Method::kInvalid;
}

std::ostream& operator<<(std::ostream& os, HttpRequest::Method method) {
  auto as_int = static_cast<size_t>(method);
  if (as_int >= ABSL_ARRAYSIZE(kMethodStrs)) {
    LOG(ERR) << "Invalid method: " << as_int;
    ABSL_ASSERT(false);
    return os;
  }

  return os << kMethodStrs[as_int];
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& request) {
  os << request.method << " " << request.target << " " << request.version
     << "\n";

  std::vector<std::string> headers;
  headers.reserve(request.header_to_value_.size());
  for (const auto& item : request.header_to_value_) {
    headers.push_back(item.first);
  }

  std::sort(headers.begin(), headers.end());
  for (const auto& header : headers) {
    os << header << ":" << request.header_to_value_.at(header) << "\n";
  }

  return os;
}
