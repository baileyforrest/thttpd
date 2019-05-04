#ifndef BASE_STDIN_READER_
#define BASE_STDIN_READER_

#include <cstdio>

#include "base/err.h"
#include "base/reader.h"

class StdinReader : public Reader {
 public:
  StdinReader() = default;
  StdinReader(const StdinReader&) = delete;
  StdinReader& operator=(const StdinReader&) = delete;
  ~StdinReader() override = default;

  Result<ssize_t> Read(absl::Span<char> buf) override {
    if (ferror(stdin)) {
      return BuildPosixErr("fread failed");
    }

    if (feof(stdin)) {
      return -1;
    }

    return fread(buf.data(), 1, buf.size(), stdin);
  }
};

#endif  // BASE_STDIN_READER_
