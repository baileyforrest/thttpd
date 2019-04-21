#pragma once

#include <iostream>
#include <sstream>

class Logger {
 public:
  enum class Type {
    INFO,
    ERR,
  };

  Logger(Type type, const char* file, int line);
  ~Logger();

  std::ostream& stream() { return stream_; }

 private:
  static const char* TypeString(Type type);

  const Type type_;
  std::ostringstream stream_;
};

#define LOG(type) Logger(Logger::Type::type, __FILE__, __LINE__).stream()
