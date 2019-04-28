#ifndef MAIN_REQUEST_PARSER_H_
#define MAIN_REQUEST_PARSER_H_

#include <string>
#include <utility>

#include "absl/strings/string_view.h"
#include "main/http-request.h"

// HTTP request parser. See https://tools.ietf.org/html/rfc7230
class RequestParser {
 public:
  enum class State {
    kInvalid,
    kPending,
    kReady,
  };

  RequestParser() = default;

  // Add data to the current request. Returns the current state of the parser.
  State AddData(absl::string_view data);

  // If |AddData| returns |kReady|, returns the last parsed request and resets
  // the parser to parse the next request.
  HttpRequest GetRequestAndReset();

 private:
  void Reset();
  State ProcessLine(absl::string_view line);

  std::string buf_;
  HttpRequest current_request_;
};

#endif  // MAIN_REQUEST_PARSER_H_
