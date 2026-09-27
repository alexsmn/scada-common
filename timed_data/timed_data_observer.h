#pragma once

class PropertySet;

class TimedDataObserver {
 public:
  virtual ~TimedDataObserver() = default;
  virtual void OnTimedDataNodeModified() {}
  virtual void OnTimedDataDeleted() {}
  virtual void OnEventsChanged() {}
  virtual void OnPropertyChanged(const PropertySet& properties) {}
};
