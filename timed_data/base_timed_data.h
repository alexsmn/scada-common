#pragma once

#include "base/observer_list.h"
#include "timed_data/timed_data.h"

namespace rt {

class BaseTimedData : public TimedData {
 public:
  BaseTimedData();
  ~BaseTimedData();

  // TimedData
  virtual void SetFrom(base::Time from) final;
  virtual base::Time GetFrom() const final { return from_; }
  virtual base::Time GetReadyFrom() const final { return ready_from_; }
  virtual bool IsAlerting() const final { return alerting_; }
  virtual scada::DataValue GetDataValue() const final { return current_; }
  virtual base::Time GetChangeTime() const final { return change_time_; }
  virtual const TimedVQMap* GetValues() const final { return &map_; }
  virtual scada::DataValue GetValueAt(const base::Time& time) const final;
  virtual void AddObserver(TimedDataDelegate& observer) final;
  virtual void RemoveObserver(TimedDataDelegate& observer) final;
  virtual NodeRef GetNode() const { return nullptr; }
  virtual const events::EventSet* GetEvents() const { return nullptr; }
  virtual void Acknowledge() {}
  virtual void Write(double value,
                     const scada::WriteFlags& flags,
                     const StatusCallback& callback) const;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const StatusCallback& callback) const;

 protected:
  bool historical() const { return from_ != kTimedDataCurrentOnly; }

  // Update |ready_from_| to new |time| if new time is earlier, and also
  // update |from_| if |time| is earlier than it.
  void UpdateReadyFrom(base::Time time);
  // Used when data is disconnected.
  void ResetReadyFrom();
  // |from_| parameter was changed. Data needs to be updated.
  virtual void OnFromChanged() = 0;

  void NotifyTimedDataCorrection(size_t count, const scada::DataValue* tvqs);
  void NotifyDataReady();
  void NotifyPropertyChanged(const PropertySet& properties);
  void NotifyEventsChanged();

  bool UpdateCurrent(const scada::DataValue& value);
  bool UpdateMap(const scada::DataValue& value);

  void ClearRange(base::Time from, base::Time to);

  void Delete();
  void Failed();

  // TODO: Cannot it be replaced by |GetEvents() && !GetEvents()->empty()|?
  bool alerting_;

  scada::DataValue current_;
  base::Time change_time_;

  base::ObserverList<TimedDataDelegate> observers_;

  base::Time from_;

  // Requested historical range. kTimedDataCurrentOnly if is not ready at all.
  base::Time ready_from_;

  TimedVQMap map_;

  DISALLOW_COPY_AND_ASSIGN(BaseTimedData);
};

}  // namespace rt