#pragma once

#include "timed_data/timed_data_property.h"

namespace scada {
class DataValue;
}

namespace rt {

class TimedDataDelegate {
 public:
  virtual void OnTimedDataCorrections(size_t count, const scada::DataValue* tvqs) {}
  virtual void OnTimedDataReady() {}
  virtual void OnTimedDataNodeModified() {}
  virtual void OnTimedDataDeleted() {}
  virtual void OnEventsChanged() {}
  virtual void OnPropertyChanged(const PropertySet& properties) {}
};

} // namespace rt
