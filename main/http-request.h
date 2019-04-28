#ifndef MAIN_HTTP_REQUEST_H_
#define MAIN_HTTP_REQUEST_H_

#include <iostream>
#include <string>

#include "absl/container/flat_hash_map.h"

struct HttpRequest {
  enum class Method {
    kGet,

    kInvalid,  // Must be last.
  };

  // Returns kInvalid if |text| is not a valid method.
  static Method ParseMethod(absl::string_view text);

  Method method = Method::kInvalid;
  std::string target;
  std::string version;
  absl::flat_hash_map<std::string, std::string> header_to_value_;
};

std::ostream& operator<<(std::ostream& os, HttpRequest::Method method);
std::ostream& operator<<(std::ostream& os, const HttpRequest& request);

#endif  // MAIN_HTTP_REQUEST_H_
