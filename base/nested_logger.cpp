#include "base/strings/stringprintf.h"
#include "base/nested_logger.h"

#include <cassert>

NestedLogger::NestedLogger() {
}

NestedLogger::NestedLogger(std::shared_ptr<const Logger> parent, const std::string& prefix)
    : parent_(parent) {
  set_prefix(prefix);
}

void NestedLogger::set_prefix(const std::string& prefix) {
  if (!prefix.empty())
    prefix_ = prefix + ": ";
  else
    prefix_.clear();
}

void NestedLogger::Write(LogSeverity severity, const char* message) const {
  if (parent_)
    parent_->Write(severity, (prefix_ + message).c_str());
}

void NestedLogger::WriteV(LogSeverity severity, const char* format, va_list args) const {
  if (parent_)
    parent_->Write(severity, (prefix_ + base::StringPrintV(format, args)).c_str());
}

void NestedLogger::WriteF(LogSeverity severity, const char* format, ...) const {
  if (parent_) {
    va_list args;
    va_start(args, format);
    parent_->Write(severity, (prefix_ + base::StringPrintV(format, args)).c_str());
    va_end(args);
  }
}
