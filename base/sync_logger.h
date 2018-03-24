#pragma once

#include "base/logger.h"
#include "base/strings/stringprintf.h"

class SyncLogger final : public Logger {
 public:
  SyncLogger(std::shared_ptr<const Logger> logger,
             scoped_refptr<base::TaskRunner> task_runner)
      : logger_{std::move(logger)}, task_runner_{std::move(task_runner)} {}

  virtual void Write(LogSeverity severity, const char* message) const override {
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&SyncLogger::WriteHelper, logger_, severity,
                              std::string{message}));
  }

  virtual void WriteV(LogSeverity severity,
                      const char* format,
                      va_list args) const override {
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&SyncLogger::WriteHelper, logger_, severity,
                              base::StringPrintV(format, args)));
  }

  virtual void WriteF(LogSeverity severity,
                      const char* format,
                      ...) const override {
    va_list args;
    va_start(args, format);
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&SyncLogger::WriteHelper, logger_, severity,
                              base::StringPrintV(format, args)));
    va_end(args);
  }

 private:
  static void WriteHelper(const std::shared_ptr<const Logger>& logger,
                          LogSeverity severity,
                          const std::string& message) {
    logger->Write(severity, message.c_str());
  }

  const std::shared_ptr<const Logger> logger_;
  const scoped_refptr<base::TaskRunner> task_runner_;
};
