#pragma once

#ifdef OS_WIN
#include <windows.h>
#endif

#include "base/base_export.h"
#include "base/logger.h"

class BASE_EXPORT ConsoleLogger : public Logger {
 public:
  ConsoleLogger();

  // Logger
  virtual void Write(LogSeverity severity, const char* message) const override;
  virtual void WriteV(LogSeverity severity, const char* format, va_list args) const override;
  virtual void WriteF(LogSeverity severity, const char* format, ...) const override;

  static const ConsoleLogger& GetInstance() {
    static const ConsoleLogger s_logger;
    return s_logger;
  }

 private:
#ifdef OS_WIN
  HANDLE console_;
  WORD initial_attributes_;
#endif
};
