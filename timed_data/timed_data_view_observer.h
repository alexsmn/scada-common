#pragma once

#include <span>

namespace scada {
class DataValue;
}

class TimedDataViewObserver {
 public:
  // TODO: This should just report a `scada::DateTimeRange`.
  virtual void OnTimedDataUpdates(std::span<const scada::DataValue> values) {}
  virtual void OnTimedDataReady() {}
};
