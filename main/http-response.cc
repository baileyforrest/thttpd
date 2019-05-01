#include "main/http-response.h"

#include <ctime>
#include <utility>

#include "absl/base/macros.h"
#include "base/err.h"
#include "base/logging.h"

namespace {

constexpr char kVersion[] = "HTTP/1.1";
constexpr char kNewline[] = "\r\n";
constexpr char kServerName[] = "thttpd";

Result<std::string> GetDateStringNow() {
  time_t now = time(nullptr);
  struct tm tm;
  if (gmtime_r(&now, &tm) == nullptr) {
    return Err("gmtime_r failed");
  }

  // TODO(bcf): Day of week and month name are loacle dependent, but meh.
  char buf[256];
  if (strftime(buf, sizeof(buf), "%a, %e %b %Y %H:%M:%S GMT", &tm) == 0) {
    return Err("strftime failed");
  }

  return std::string(buf);
}

}  // namespace

// rfc7231 - 6.1.  Overview of Status Codes
// static
const char* HttpResponse::CodeToString(Code code) {
  switch (code) {
    case Code::kOk:
      return "OK";
    case Code::kBadRequest:
      return "Bad Request";
    case Code::kNotFound:
      return "Not Found";
    case Code::kInternalServerError:
      return "Internal Server Error";
  }

  ABSL_ASSERT(false);
  return "";
}

// static
HttpResponse HttpResponse::BuildWithDefaultHeaders(
    Code code, const std::map<std::string, std::string>& additional_headers) {
  HttpResponse result;
  result.code = code;

  auto date_str = GetDateStringNow();
  if (!date_str.ok()) {
    LOG(WARN) << "Failed to get date: " << date_str.err();
  } else {
    result.header_to_value_.emplace("Date", std::move(*date_str));
  }

  result.header_to_value_.emplace("Server", kServerName);
  result.header_to_value_.emplace("Connection", "keep-alive");

  for (const auto& item : additional_headers) {
    result.header_to_value_[item.first] = item.second;
  }

  return result;
}

std::ostream& operator<<(std::ostream& os, HttpResponse::Code code) {
  return os << HttpResponse::CodeToString(code);
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& response) {
  os << kVersion << " " << static_cast<int>(response.code) << " "
     << response.code << kNewline;
  for (const auto& item : response.header_to_value_) {
    os << item.first << ": " << item.second << kNewline;
  }

  return os << kNewline;
}
