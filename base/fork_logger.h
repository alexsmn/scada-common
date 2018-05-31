#pragma once

#include "base/logger.h"

#include <vector>

class ForkLogger : public Logger {
 public:
  virtual ~ForkLogger();
 
  void AddLogger(const Logger& logger, bool own);
 
  // Logger
  virtual void Write(LogSeverity severity, const char* message) const override;
  virtual void WriteV(LogSeverity severity, const char* format, va_list args) const override;
  virtual void WriteF(LogSeverity severity, const char* format, ...) const override;
  
 private:
  typedef std::vector<std::pair<const Logger*, bool> > Loggers;
  Loggers loggers_;
};
