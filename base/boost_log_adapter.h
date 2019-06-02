#pragma once

#include "base/boost_log.h"
#include "base/logger.h"
#include "base/strings/stringprintf.h"

class BoostLogAdapter : public Logger {
 public:
  explicit BoostLogAdapter(const std::string& name) : logger_{LOG_NAME(name)} {}

  virtual void Write(LogSeverity severity, const char* message) const override {
    BOOST_LOG_SEV(logger_, ToBoostLogSeverity(severity)) << message;
  }

  virtual void WriteV(LogSeverity severity,
                      const char* format,
                      va_list args) const override {
    BOOST_LOG_SEV(logger_, ToBoostLogSeverity(severity))
        << base::StringPrintV(format, args);
  }

  virtual void WriteF(LogSeverity severity,
                      const char* format,
                      ...) const override {
    va_list args;
    va_start(args, format);
    BOOST_LOG_SEV(logger_, ToBoostLogSeverity(severity))
        << base::StringPrintV(format, args);
    va_end(args);
  }

 private:
  static BoostLogSeverity ToBoostLogSeverity(LogSeverity severity) {
    switch (severity) {
      case LogSeverity::Normal:
        return BoostLogSeverity::Info;
      case LogSeverity::Warning:
        return BoostLogSeverity::Warning;
      case LogSeverity::Error:
        return BoostLogSeverity::Error;
      case LogSeverity::Fatal:
        return BoostLogSeverity::Critical;
      default:
        assert(false);
        return BoostLogSeverity::Info;
    }
  }

  mutable BoostLogger logger_;
};
