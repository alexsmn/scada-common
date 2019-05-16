#pragma once

#include "base/time/time.h"

inline base::TimeDelta TimeDeltaFromSecondsF(double dt) {
  return base::TimeDelta::FromMicroseconds(static_cast<int64>(
      dt * static_cast<double>(base::Time::kMicrosecondsPerSecond)));
}

std::string FormatTimeDelta(base::TimeDelta delta);
bool ParseTimeDelta(base::StringPiece str, base::TimeDelta& delta);

base::string16 FormatTime16(base::Time time, bool utc);
bool ParseTime(base::StringPiece str, base::Time& time, bool utc);
