#include "main/request-parser.h"

#include <algorithm>
#include <locale>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "base/logging.h"

namespace {

constexpr char kLineEnd[] = "\r\n";
constexpr char kVersionPrefix[] = "HTTP/1";

}  // namespace

RequestParser::State RequestParser::AddData(absl::string_view data) {
  buf_.append(data.begin(), data.end());

  auto state = State::kPending;
  size_t offset = 0;
  while (true) {
    size_t old_offset = offset;
    offset = buf_.find(kLineEnd, old_offset);
    if (offset == std::string::npos) {
      break;
    }

    size_t len = offset - old_offset;
    absl::string_view line(buf_.data() + old_offset, len);
    offset += strlen(kLineEnd);

    state = ProcessLine(line);
    if (state == State::kInvalid) {
      Reset();
    }
    if (state != State::kPending) {
      break;
    }
  }

  // Move unused data to the beginning.
  size_t remain = offset - buf_.size();
  memcpy(&buf_[0], buf_.data() + offset, remain);
  buf_.resize(remain);

  return state;
}

HttpRequest RequestParser::GetRequestAndReset() {
  auto ret = std::move(current_request_);
  current_request_ = {};
  return ret;
}

void RequestParser::Reset() {
  buf_.clear();
  current_request_ = {};
}

RequestParser::State RequestParser::ProcessLine(absl::string_view line) {
  bool has_request_line =
      current_request_.method != HttpRequest::Method::kInvalid;

  // Empty line signals end of the request. It's valid if we got a valid request
  // line.
  if (line.size() == 0) {
    if (!has_request_line) {
      VLOG(1) << "Request is missing request line";
      return State::kInvalid;
    }
    return State::kReady;
  }

  // If we don't have a request line, then the first line must be a request
  // line.
  if (!has_request_line) {
    // rfc7230 - 3.1.1.  Request Line
    for (auto token : absl::StrSplit(line, ' ', absl::SkipEmpty())) {
      if (!current_request_.version.empty()) {
        VLOG(1) << "Request line trailing characters: " << line;
        return State::kInvalid;
      }

      // Method.
      if (current_request_.method == HttpRequest::Method::kInvalid) {
        auto method = HttpRequest::ParseMethod(token);
        if (method == HttpRequest::Method::kInvalid) {
          VLOG(1) << "Invalid request method: " << token
                  << ". Request: " << line;
          return State::kInvalid;
        }
        current_request_.method = method;
        continue;
      }

      // Target. We only support origin-form now.
      if (current_request_.target.empty()) {
        if (token[0] != '/') {
          VLOG(1) << "Invalid request-target: " << token
                  << ". Request: " << line;
          return State::kInvalid;
        }
        current_request_.target = std::string(token);
        continue;
      }

      if (current_request_.version.empty()) {
        if (token.find(kVersionPrefix) != 0) {
          VLOG(1) << "Expected version prefix: " << kVersionPrefix
                  << ". Got: " << token << ". Request: " << line;
          return State::kInvalid;
        }
        current_request_.version = std::string(token);
        continue;
      }
    }

    return State::kPending;
  }

  // Parse a header.
  // rfc7230 - 3.2.  Header Fields

  std::locale c_locale("C");
  size_t colon_idx = 0;
  for (; colon_idx < line.size(); ++colon_idx) {
    char c = line[colon_idx];
    if (!std::isprint(c, c_locale)) {
      VLOG(1) << "Invalid character while parsing header: " << line;
      return State::kInvalid;
    }
    // rfc7230 - 3.2.4.  Field Parsing
    // No whitespace is allowed between the header field-name and colon.
    if (std::isspace(c, c_locale)) {
      VLOG(1) << "Space before colon in header: " << line;
      return State::kInvalid;
    }
    if (c == ':') {
      break;
    }
  }

  auto header_name = std::string(line.substr(0, colon_idx));
  size_t value_start = std::min(colon_idx + 1, line.size());
  auto header_value = line.substr(value_start);

  auto& value = current_request_.header_to_value_[header_name];
  if (value.empty()) {
    value = std::string(header_value);
  } else {
    // RFC2616 - Multiple message-header fields with the same field-name MAY be
    // present in a message if and only if the entire field-value for that
    // header field is defined as a comma-separated list.
    absl::StrAppend(&value, ", ", header_value);
  }

  return State::kPending;
}
