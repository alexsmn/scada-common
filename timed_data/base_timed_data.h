#pragma once

#include "base/boost_log.h"
#include "base/observer_list.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_view.h"

class PropertySet;

class BaseTimedData : public TimedData {
 public:
  BaseTimedData();
  ~BaseTimedData();

  BaseTimedData(const BaseTimedData&) = delete;
  BaseTimedData& operator=(const BaseTimedData&) = delete;

  // TimedData
  virtual const std::vector<scada::DateTimeRange>& GetReadyRanges()
      const override {
    return timed_data_view_.ready_ranges();
  }
  virtual bool IsAlerting() const override { return alerting_; }
  virtual scada::DataValue GetDataValue() const override { return current_; }
  virtual scada::DateTime GetChangeTime() const override {
    return change_time_;
  }
  virtual std::span<const scada::DataValue> GetValues() const override {
    return timed_data_view_.values();
  }
  virtual const scada::DataValue* GetValueAt(
      const scada::DateTime& time) const override;
  virtual void AddObserver(TimedDataObserver& observer) override;
  virtual void RemoveObserver(TimedDataObserver& observer) override;
  virtual void AddViewObserver(TimedDataViewObserver& observer,
                               const scada::DateTimeRange& range) override;
  virtual void RemoveViewObserver(TimedDataViewObserver& observer) override;
  virtual NodeRef GetNode() const override { return nullptr; }
  virtual const EventSet* GetEvents() const override { return nullptr; }
  virtual void Acknowledge() override {}
  virtual std::string DumpDebugInfo() const override;

 protected:
  void NotifyPropertyChanged(const PropertySet& properties);
  void NotifyEventsChanged();

  bool historical() const { return historical_; }

  bool UpdateCurrent(const scada::DataValue& value);

  void Delete();
  void Failed();

  void UpdateObservedRanges(bool has_observers);

  virtual void OnObservedRangesChanged() {}

  TimedDataView timed_data_view_;

  // Populate history with currents. Turns to this mode on a first historical
  // subscription, and then never changes back.
  bool historical_ = false;

  // TODO: Cannot it be replaced by |GetEvents() && !GetEvents()->empty()|?
  bool alerting_ = false;

  scada::DataValue current_;
  scada::DateTime change_time_;

  base::ObserverList<TimedDataObserver> observers_;

  inline static BoostLogger logger_{LOG_NAME("TimedData")};
};
