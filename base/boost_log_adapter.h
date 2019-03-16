#pragma once

#include "base/boost_log.h"
#include "base/logger.h"

class BoostLogAdapter : public Logger {
 public:
  explicit BoostLogAdapter(const std::string& name) : logger_{LOG_NAME(name)} {}

  virtual void Write(LogSeverity severity, const char* message) const override {
  }

  virtual void WriteV(LogSeverity severity,
                      const char* format,
                      va_list args) const override {}

  virtual void WriteF(LogSeverity severity,
                      const char* format,
                      ...) const override {}

 private:
  BoostLogger logger_;
};
