#pragma once

#include <cassert>
#include <functional>
#include <set>

#include "base/time/time.h"
#include "core/data_value.h"
#include "core/status.h"
#include "core/write_flags.h"
#include "common/timed_data/timed_vq_map.h"
#include "common/timed_data/timed_data_delegate.h"
#include "common/node_ref.h"

namespace rt {

class TimedDataCache;
class TimedDataDelegate;
class PropertySet;
class TimedDataSpec;

extern const char kConnectingName[];
extern const base::Time kTimedDataCurrentOnly;

class TimedData {
 public:
  TimedData();
  virtual ~TimedData();

  void SetFrom(base::Time from);

  bool alerting() const { return alerting_; }
  const scada::DataValue& current() const { return current_; }
  base::Time change_time() const { return change_time_; }
  bool historical() const { return from_ != kTimedDataCurrentOnly; }

  base::Time from() const { return from_; }
  base::Time ready_from() const { return ready_from_; }
  
  const TimedVQMap& values() const { return map_; }

  scada::DataValue GetValueAt(const base::Time& time) const;

  void Subscribe(TimedDataSpec& spec);
  void Unsubscribe(TimedDataSpec& spec);

  virtual std::string GetFormula(bool aliases) const = 0;
  virtual base::string16 GetTitle() const = 0;
  virtual NodeRef GetNode() const { return {}; }
  virtual const events::EventSet* GetEvents() const { return NULL; }
  // Acknowledge all active events related to this data.
  virtual void Acknowledge() {}
  // Write item value.
  typedef std::function<void(const scada::Status&)> StatusCallback;
  virtual void Write(double value, const scada::WriteFlags& flags,
                        const StatusCallback& callback) const;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const StatusCallback& callback) const;

 protected:
  typedef std::set<TimedDataSpec*> TimedDataSpecSet;

  // Update |ready_from_| to new |time| if new time is earlier, and also
  // update |from_| if |time| is earlier than it.
  void UpdateReadyFrom(base::Time time);
  // Used when data is disconnected.
  void ResetReadyFrom();
  
  void NotifyTimedDataCorrection(size_t count, const scada::DataValue* tvqs);
  void NotifyDataReady();
  void NotifyPropertyChanged(const PropertySet& properties);
  void NotifyEventsChanged(const events::EventSet& events);
  
  bool UpdateCurrent(const scada::DataValue& tvq);
  bool UpdateMap(const scada::DataValue& tvq);

	void ClearRange(base::Time from, base::Time to);

  void Delete();
  void Failed();

  // TODO: Cannot it be replaced by |GetEvents() && !GetEvents()->empty()|?
  bool alerting_;

  // |from_| parameter was changed. Data needs to be updated.
  virtual void OnFromChanged() { }

  scada::DataValue current_;
  base::Time change_time_;

  TimedDataSpecSet specs_;

 private:
  friend class TimedDataCache;

  base::Time from_;

  // Requested historical range. kTimedDataCurrentOnly if is not ready at all.
  base::Time ready_from_;

	TimedVQMap map_;

  DISALLOW_COPY_AND_ASSIGN(TimedData);
};

} // namespace rt
