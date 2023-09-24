#pragma once

namespace scada {
class DataValue;
}

class PropertySet;

class TimedDataObserver {
 public:
  virtual void OnTimedDataCorrections(size_t count,
                                      const scada::DataValue* tvqs) {}
  virtual void OnTimedDataReady() {}
  virtual void OnTimedDataNodeModified() {}
  virtual void OnTimedDataDeleted() {}
  virtual void OnEventsChanged() {}
  virtual void OnPropertyChanged(const PropertySet& properties) {}
};
