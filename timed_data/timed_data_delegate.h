#pragma once

#include "timed_data/timed_data_property.h"

namespace scada {

class DataValue;

using PropertyIds = std::vector<scada::NodeId>;

} // namepspace scada

namespace events {
class EventSet;
}

namespace rt {

class TimedDataSpec;

class TimedDataDelegate {
 public:
  virtual void OnTimedDataCorrections(TimedDataSpec& spec, size_t count, const scada::DataValue* tvqs) {}
  virtual void OnTimedDataReady(TimedDataSpec& spec) {}
  virtual void OnTimedDataNodeModified(rt::TimedDataSpec& spec, const scada::PropertyIds& property_ids) {}
  virtual void OnTimedDataDeleted(TimedDataSpec& spec) {}
  virtual void OnEventsChanged(TimedDataSpec& spec, const events::EventSet& events) {}
  virtual void OnPropertyChanged(TimedDataSpec& spec, const PropertySet& properties) {}
};

} // namespace rt

