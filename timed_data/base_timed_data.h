#pragma once

#include "base/logger.h"
#include "base/observer_list.h"
#include "timed_data/timed_data.h"

namespace rt {

class PropertySet;

class BaseTimedData : public TimedData {
 public:
  explicit BaseTimedData(std::shared_ptr<const Logger> logger);
  ~BaseTimedData();

  // TimedData
  virtual const std::vector<scada::DateTimeRange>& GetReadyRanges()
      const override {
    return ready_ranges_;
  }
  virtual bool IsAlerting() const override { return alerting_; }
  virtual scada::DataValue GetDataValue() const override { return current_; }
  virtual base::Time GetChangeTime() const override { return change_time_; }
  virtual const DataValues* GetValues() const override { return &values_; }
  virtual scada::DataValue GetValueAt(const base::Time& time) const override;
  virtual void AddObserver(TimedDataDelegate& observer,
                           const scada::DateTimeRange& range) override;
  virtual void RemoveObserver(TimedDataDelegate& observer) override;
  virtual NodeRef GetNode() const override { return nullptr; }
  virtual const events::EventSet* GetEvents() const override { return nullptr; }
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

  // Update |ready_from_| to new |time| if new time is earlier.
  void SetReady(const scada::DateTimeRange& range);

  void RebuildRanges();
  void UpdateRanges();
  std::optional<scada::DateTimeRange> FindNextGap() const;
  // |ranges_| and |from_| were changed. Data needs to be updated.
  virtual void OnRangesChanged() = 0;

  void NotifyTimedDataCorrection(size_t count, const scada::DataValue* tvqs);
  void NotifyDataReady();
  void NotifyPropertyChanged(const PropertySet& properties);
  void NotifyEventsChanged();

  bool UpdateCurrent(const scada::DataValue& value);
  bool UpdateHistory(const scada::DataValue& value);

  void Clear(const scada::DateTimeRange& range);

  void Delete();
  void Failed();

  const std::shared_ptr<const Logger> logger_;

  // TODO: Cannot it be replaced by |GetEvents() && !GetEvents()->empty()|?
  bool alerting_ = false;

  scada::DataValue current_;
  base::Time change_time_;

  base::ObserverList<TimedDataDelegate> observers_;
  std::map<TimedDataDelegate*, scada::DateTimeRange> observer_ranges_;
  std::vector<scada::DateTimeRange> ranges_;

  // Populate history with currents. Turns to this mode on a first historical
  // subscription, and then never changes back.
  bool historical_ = false;

  std::vector<scada::DateTimeRange> ready_ranges_;

  DataValues values_;

  DISALLOW_COPY_AND_ASSIGN(BaseTimedData);
};

}  // namespace rt
