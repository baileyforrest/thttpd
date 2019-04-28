#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <limits>

#include "base/logging.h"
#include "base/util.h"
#include "main/thttpd.h"

int main(int argc, char** argv) {
  if (argc < 3) {
    LOG(ERR) << "Usage: " << argv[0] << " port path_to_serve";
    return EXIT_FAILURE;
  }

  int port;
  if (!absl::SimpleAtoi(argv[1], &port)) {
    LOG(ERR) << "Failed to parse port";
    return EXIT_FAILURE;
  }

  if (port > std::numeric_limits<uint16_t>::max()) {
    LOG(ERR) << "Invalid port number: " << port;
    return EXIT_FAILURE;
  }

  std::string path_to_serve = argv[2];
  struct stat stat_buf;
  if (stat(path_to_serve.c_str(), &stat_buf) < 0) {
    LOG(ERR) << "Failed to stat " << path_to_serve << ": " << strerror(errno);
    return EXIT_FAILURE;
  }

  if (!S_ISDIR(stat_buf.st_mode)) {
    LOG(ERR) << "Not a directory: " << path_to_serve;
    return EXIT_FAILURE;
  }

  auto path_or = util::CanonicalizePath(path_to_serve);
  if (!path_or.ok()) {
    LOG(ERR) << path_or.err();
    return EXIT_FAILURE;
  }
  path_to_serve = std::move(*path_or);

  // TODO(bcf): Add flag for this.
  gVerboseLogLevel = 4;

  Config config;
  config.port = port;
  config.path_to_serve = path_to_serve;

  auto thttpd_or = Thttpd::Create(config);
  if (!thttpd_or.ok()) {
    LOG(ERR) << "Failed to create Thttpd: " << thttpd_or.err();
    return EXIT_FAILURE;
  }
  auto result = thttpd_or.result()->Start();
  if (!result.ok()) {
    LOG(ERR) << "Failed to run Thttpd: " << result.err();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
