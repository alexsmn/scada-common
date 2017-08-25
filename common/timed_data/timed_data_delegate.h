#pragma once

#include "core/node_observer.h"
#include "common/timed_data/timed_data_property.h"

namespace scada {
class DataValue;
}

namespace events {
class EventSet;
}

namespace memdb {
class TableUpdateInfo;
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
