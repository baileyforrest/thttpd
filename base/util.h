#ifndef _BASE_UTIL_H_
#define _BASE_UTIL_H_

#include <string>

#include "base/err.h"

namespace util {

// Canonicalize a unix path (e.g. remove ..).
Result<std::string> CanonicalizePath(const std::string& input);

}  // namespace util

#endif  // _BASE_UTIL_H_
