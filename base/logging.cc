#include <cassert>
#include <ctime>
#include <iomanip>
#include <string>

#include "base/logging.h"

// static
const char* Logger::TypeString(Type type) {
  switch (type) {
    case Type::INFO:
      return "INFO";
    case Type::WARN:
      return "WARN";
    case Type::ERR:
      return "ERR";
  }
  assert(false);
  return "";
}

Logger::Logger(Type type, const char* file, int line) : type_(type) {
  std::string filename = file;
  auto pos = filename.find_last_of("/");
  if (pos != std::string::npos) {
    filename = filename.substr(pos + 1);
  }

  std::time_t now = std::time(nullptr);
  struct tm now_tm;
  localtime_r(&now, &now_tm);

  stream_ << "[" << std::put_time(&now_tm, "%m%e/%T") << ":" << TypeString(type)
          << ":" << filename << "(" << line << ")] ";
}

Logger::~Logger() {
  auto& out_stream = type_ == Type::ERR ? std::cerr : std::cout;
  out_stream << stream_.str() << "\n";
}
