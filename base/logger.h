#pragma once

#include "base/files/file_path.h"

#include <memory>
#include <stdarg.h>
#include <string>

enum class LogSeverity {
  Normal = 0,
  Warning = 1,
  Error = 2,
  Fatal = 3,
  Count = 4
};

class Logger {
 public:
  virtual ~Logger() { }
 
  virtual void Write(LogSeverity severity, const char* message) const = 0;
  virtual void WriteV(LogSeverity severity, const char* format, va_list args) const = 0;
  virtual void WriteF(LogSeverity severity, const char* format, ...) const = 0;
};

class NullLogger : public Logger {
 public:
  virtual void Write(LogSeverity severity, const char* message) const override { }
  virtual void WriteV(LogSeverity severity, const char* format, va_list args) const override { }
  virtual void WriteF(LogSeverity severity, const char* format, ...) const override { }

  static std::shared_ptr<NullLogger> GetInstance() {
    static auto logger = std::make_shared<NullLogger>();
    return logger;
  }
};

std::unique_ptr<Logger> CreateFileLogger(int path_service_key, base::FilePath::StringType base_name);
