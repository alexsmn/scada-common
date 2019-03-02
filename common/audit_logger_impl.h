#pragma once

#include "base/logger.h"
#include "common/audit_logger.h"

class AuditLoggerImpl : public AuditLogger {
 public:
  explicit AuditLoggerImpl(std::shared_ptr<Logger> logger)
      : logger_{std::move(logger)} {}

  virtual void Log(const char* name, const Metric<Duration>& metric) override {
    logger_->WriteF(
        LogSeverity::Normal,
        "%s: Min: %llu, Max: %llu, Mean: %llu, Total: %llu, Count: %Iu", name,
        std::chrono::duration_cast<std::chrono::microseconds>(metric.min())
            .count(),
        std::chrono::duration_cast<std::chrono::microseconds>(metric.max())
            .count(),
        std::chrono::duration_cast<std::chrono::microseconds>(metric.mean())
            .count(),
        std::chrono::duration_cast<std::chrono::microseconds>(metric.total())
            .count(),
        metric.count());
  }

 private:
  const std::shared_ptr<Logger> logger_;
};
