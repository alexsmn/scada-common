#pragma once

#include "base/metric.h"

#include <chrono>

class AuditLogger {
 public:
  virtual ~AuditLogger() {}

  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;

  virtual void Log(const char* name, const Metric<Duration>& metric) = 0;
};
