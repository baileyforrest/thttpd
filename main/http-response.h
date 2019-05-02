#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include <iostream>
#include <map>
#include <string>

#include "base/err.h"

// Http response header.
// rfc7231
struct HttpResponse {
  enum class Code {
    kOk = 200,
    kBadRequest = 400,
    kNotFound = 404,
    kInternalServerError = 500,
  };

  static const char* CodeToString(Code code);
  static HttpResponse BuildWithDefaultHeaders(
      Code code, const std::map<std::string, std::string>& additional_headers);
  static Result<std::string> FormatTime(time_t time_val);

  Code code = Code::kInternalServerError;
  std::map<std::string, std::string> header_to_value_;
};

std::ostream& operator<<(std::ostream& os, HttpResponse::Code code);
std::ostream& operator<<(std::ostream& os, const HttpResponse& response);

#endif  // _HTTP_RESPONSE_H_
