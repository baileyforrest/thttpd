#ifndef BASE_READER_H_
#define BASE_READER_H_

#include "absl/types/span.h"
#include "base/err.h"

class Reader {
 public:
  virtual ~Reader() = default;

  // Returns number of bytes read int |buf|. Returns -1 on EOF.
  virtual Result<ssize_t> Read(absl::Span<char> buf) = 0;
};

#endif  // BASE_READER_H_
