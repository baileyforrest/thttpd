#ifndef ZLIB_DEFLATE_READER_H_
#define ZLIB_DEFLATE_READER_H_

#include <zlib.h>

#include <memory>

#include "base/err.h"
#include "base/reader.h"

// Applies deflate compression to the provided reader.
class ZlibDeflateReader : public Reader {
 public:
  static Result<std::unique_ptr<ZlibDeflateReader>> Create(Reader* reader);

  ZlibDeflateReader(const ZlibDeflateReader&) = delete;
  ZlibDeflateReader& operator=(const ZlibDeflateReader&) = delete;
  ~ZlibDeflateReader();

  // Reader implementation:
  Result<ssize_t> Read(absl::Span<char> buf) override;

 private:
  enum {
    kChunkSize = 16384,
  };

  explicit ZlibDeflateReader(Reader* reader);
  Result<void> Init();

  Reader* const reader_;
  z_stream stream_{};
  char in_[kChunkSize];
  int flush_ = 0;
  bool reader_eof_ = false;
  bool eof_ = false;
};

#endif  // ZLIB_DEFLATE_READER_H_
