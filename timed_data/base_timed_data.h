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
  virtual base::Time GetChangeTime() const override { return change_time_; }
  virtual const DataValues* GetValues() const override {
    return &timed_data_view_.values();
  }
  virtual scada::DataValue GetValueAt(const base::Time& time) const override;
  virtual void AddObserver(TimedDataDelegate& observer,
                           const scada::DateTimeRange& range) override;
  virtual void RemoveObserver(TimedDataDelegate& observer) override;
  virtual NodeRef GetNode() const override { return nullptr; }
  virtual const EventSet* GetEvents() const override { return nullptr; }
  virtual void Acknowledge() override {}
  virtual void Write(double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const StatusCallback& callback) const override;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback) const override;
  virtual std::string DumpDebugInfo() const override;

 protected:
  bool historical() const { return historical_; }

  bool UpdateCurrent(const scada::DataValue& value);

  void Delete();
  void Failed();

  void UpdateRanges();

  virtual void OnRangesChanged() {}

  TimedDataView timed_data_view_;

  // Populate history with currents. Turns to this mode on a first historical
  // subscription, and then never changes back.
  bool historical_ = false;

  // TODO: Cannot it be replaced by |GetEvents() && !GetEvents()->empty()|?
  bool alerting_ = false;

  scada::DataValue current_;
  base::Time change_time_;

  inline static BoostLogger logger_{LOG_NAME("TimedData")};
};
