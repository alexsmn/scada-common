#include "base/time/time.h"

inline base::TimeDelta TimeDeltaFromSecondsF(double dt) {
  return base::TimeDelta::FromMicroseconds(static_cast<int64>(
      dt * static_cast<double>(base::Time::kMicrosecondsPerSecond)));
}
