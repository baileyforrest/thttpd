#ifndef MAIN_FILE_READER_H_
#define MAIN_FILE_READER_H_

#include <cstdio>
#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "base/err.h"

class FileReader {
 public:
  static Result<FileReader> Create(absl::string_view path);

  FileReader(FileReader&&) = default;
  FileReader& operator=(FileReader&&) = default;

  // Will have a short read on EOF.
  Result<size_t> Read(absl::Span<uint8_t> buf);

 private:
  // Use C file API because it's faster than C++ streams.
  using FilePtr = std::unique_ptr<FILE, decltype(&fclose)>;

  FileReader(std::string path, FilePtr file);

  std::string path_;
  FilePtr file_;
};

#endif  // MAIN_FILE_READER_H_
