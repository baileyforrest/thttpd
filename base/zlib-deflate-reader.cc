#include "base/zlib-deflate-reader.h"

#include <utility>

#include "absl/memory/memory.h"

namespace {

constexpr int kCompressionLevel = 6;
constexpr int kMemLevel = 8;

// 16 adds gzip header.
constexpr int kWindowBits = 16 + MAX_WBITS;

}  // namespace

// static
Result<std::unique_ptr<ZlibDeflateReader>> ZlibDeflateReader::Create(
    Reader* reader) {
  auto ret = absl::WrapUnique(new ZlibDeflateReader(reader));
  TRY(ret->Init());
  return std::move(ret);
}

ZlibDeflateReader::ZlibDeflateReader(Reader* reader) : reader_(reader) {}

ZlibDeflateReader::~ZlibDeflateReader() { deflateEnd(&stream_); }

Result<void> ZlibDeflateReader::Init() {
  if (deflateInit2(&stream_, kCompressionLevel, Z_DEFLATED, kWindowBits,
                   kMemLevel, Z_DEFAULT_STRATEGY) != Z_OK) {
    return Err("deflateInit failed");
  }

  return {};
}

Result<ssize_t> ZlibDeflateReader::Read(absl::Span<char> buf) {
  if (eof_) {
    return -1;
  }

  stream_.avail_out = buf.size();
  stream_.next_out = reinterpret_cast<unsigned char*>(buf.data());
  while (stream_.avail_out > 0 && !eof_) {
    if (!reader_eof_ && stream_.avail_in == 0) {
      ssize_t num_read = TRY(reader_->Read(in_));
      reader_eof_ = num_read == -1;
      flush_ = reader_eof_ ? Z_FINISH : Z_NO_FLUSH;
      stream_.avail_in = reader_eof_ ? 0 : num_read;
      stream_.next_in = reinterpret_cast<unsigned char*>(in_);
    }

    size_t before_avail_out = stream_.avail_out;
    if (deflate(&stream_, flush_) == Z_STREAM_ERROR) {
      return Err("deflate failed");
    }
    eof_ = reader_eof_ && (before_avail_out - stream_.avail_out == 0);
  }

  return buf.size() - stream_.avail_out;
}
