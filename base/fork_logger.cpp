#include "base/fork_logger.h"

ForkLogger::~ForkLogger() {
  for (Loggers::iterator i = loggers_.begin(); i != loggers_.end(); ++i) {
    if (i->second)
      delete i->first;
  }
}

void ForkLogger::AddLogger(const Logger& logger, bool own) {
  loggers_.push_back(std::make_pair(&logger, own));
}

void ForkLogger::Write(LogSeverity severity, const char* message) const {
  for (auto& p : loggers_)
    p.first->Write(severity, message);
}

void ForkLogger::WriteV(LogSeverity severity, const char* format, va_list args) const {
  for (auto& p : loggers_)
    p.first->WriteV(severity, format, args);
}

void ForkLogger::WriteF(LogSeverity severity, const char* format, ...) const {
  va_list args;
  va_start(args, format);
  for (auto& p : loggers_)
    p.first->WriteV(severity, format, args);
  va_end(args);
}
