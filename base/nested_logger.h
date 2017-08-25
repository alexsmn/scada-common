#pragma once

#include <memory>
#include <string>

#include "base/base_export.h"
#include "base/logger.h"

class NestedLogger : public Logger {
 public:
  NestedLogger();
  NestedLogger(std::shared_ptr<const Logger> parent, const std::string& prefix);

  const Logger* parent() const { return parent_.get(); }
  void set_parent(std::shared_ptr<const Logger> parent) { parent_ = std::move(parent); }

  void set_prefix(const std::string& prefix);

  // Logger overrides
  virtual void Write(LogSeverity severity, const char* message) const override;
  virtual void WriteV(LogSeverity severity, const char* format, va_list args) const override;
  virtual void WriteF(LogSeverity severity, const char* format, ...) const override;
  
 private:
  std::shared_ptr<const Logger> parent_;
  std::string prefix_;
};
