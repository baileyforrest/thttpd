#include <limits>

#include "base/logging.h"
#include "main/thttpd.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    LOG(ERR) << "Need port";
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

  // TODO(bcf): Add flag for this.
  gVerboseLogLevel = 4;

  Thttpd::Config config;
  config.port = port;

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
