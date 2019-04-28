#include "main/file-reader.h"

#include <utility>

// static
Result<FileReader> FileReader::Create(absl::string_view path) {
  // We must make a string because fopen requires a C string.
  std::string path_str(path);

  // Can't construct inside of the wrapped FilePtr type because fclose(NULL) is
  // undefined behavior.
  FILE* fp = fopen(path_str.c_str(), "r");
  if (!fp) {
    return BuildPosixErr(absl::StrCat("Failed to open ", path_str));
  }

  return FileReader(std::move(path_str), {fp, &fclose});
}

FileReader::FileReader(std::string path, FilePtr file)
    : path_(std::move(path)), file_(std::move(file)) {}

Result<size_t> FileReader::Read(absl::Span<uint8_t> buf) {
  size_t amount = fread(buf.data(), /*size=*/1, buf.size(), file_.get());
  if (amount != buf.size()) {
    if (ferror(file_.get())) {
      return BuildPosixErr(absl::StrCat("Read failed on ", path_));
    }
    eof_ = true;
  }

  return amount;
}
