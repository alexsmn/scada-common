#pragma once

namespace scada {
class DataValue;
}

class TimedDataViewObserver {
 public:
  virtual void OnTimedDataCorrections(size_t count,
                                      const scada::DataValue* tvqs) {}
  virtual void OnTimedDataReady() {}
};
