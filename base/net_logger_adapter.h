#pragma once

#include "base/logger.h"
#include "net/logger.h"

class NetLoggerAdapter final : public net::Logger {
 public:
  explicit NetLoggerAdapter(const ::Logger& logger) : logger_{logger} {}

  virtual void Write(net::LogSeverity severity,
                     const char* message) const override {
    logger_.Write(static_cast<::LogSeverity>(severity), message);
  }

  virtual void WriteV(net::LogSeverity severity,
                      const char* format,
                      va_list args) const override {
    logger_.WriteV(static_cast<::LogSeverity>(severity), format, args);
  }

  virtual void WriteF(net::LogSeverity severity,
                      const char* format,
                      ...) const override {
    va_list args;
    va_start(args, format);
    logger_.WriteV(static_cast<::LogSeverity>(severity), format, args);
    va_end(args);
  }

 private:
  const ::Logger& logger_;
};
