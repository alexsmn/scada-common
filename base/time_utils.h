#pragma once

#include "base/strings/string_piece.h"
#include "base/time/time.h"

inline base::TimeDelta TimeDeltaFromSecondsF(double dt) {
  return base::TimeDelta::FromMicroseconds(static_cast<int64>(
      dt * static_cast<double>(base::Time::kMicrosecondsPerSecond)));
}

std::string SerializeToString(base::TimeDelta delta);
bool Deserialize(base::StringPiece str, base::TimeDelta& delta);

std::string SerializeToString(base::Time time);
bool Deserialize(base::StringPiece str, base::Time& time);
