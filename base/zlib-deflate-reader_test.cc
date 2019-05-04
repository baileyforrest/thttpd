#include <cstdlib>

#include "base/logging.h"
#include "base/stdin-reader.h"
#include "base/zlib-deflate-reader.h"

namespace {

Result<void> Run() {
  StdinReader stdin_reader;

  auto zlib_reader = TRY(ZlibDeflateReader::Create(&stdin_reader));
  while (true) {
    char buf[BUFSIZ];
    ssize_t num_read = TRY(zlib_reader->Read(buf));
    if (num_read == -1) {
      break;
    }
    fwrite(buf, 1, num_read, stdout);
  }

  return {};
}

}  // namespace

// Simple compression program which reads from stdin and writes to stdout.
int main() {
  auto result = Run();
  if (!result.ok()) {
    LOG(ERR) << result.err();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
