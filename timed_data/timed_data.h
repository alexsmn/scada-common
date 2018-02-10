#pragma once

#include <cassert>
#include <functional>
#include <set>

#include "base/time/time.h"
#include "common/node_ref.h"
#include "core/configuration_types.h"
#include "core/data_value.h"
#include "core/status.h"
#include "core/write_flags.h"
#include "timed_data/timed_vq_map.h"

namespace events {
class EventSet;
}

namespace rt {

class PropertySet;
class TimedDataCache;
class TimedDataDelegate;
class TimedDataSpec;

extern const base::Time kTimedDataCurrentOnly;

class TimedData {
 public:
  virtual ~TimedData() {}

  virtual base::Time GetFrom() const = 0;
  virtual void SetFrom(base::Time from) = 0;
  virtual base::Time GetReadyFrom() const = 0;

  virtual scada::DataValue GetDataValue() const = 0;
  virtual base::Time GetChangeTime() const = 0;

  virtual const TimedVQMap* GetValues() const = 0;
  virtual scada::DataValue GetValueAt(const base::Time& time) const = 0;

  virtual void AddObserver(TimedDataDelegate& observer) = 0;
  virtual void RemoveObserver(TimedDataDelegate& observer) = 0;

  virtual std::string GetFormula(bool aliases) const = 0;
  virtual scada::LocalizedText GetTitle() const = 0;
  virtual NodeRef GetNode() const = 0;

  virtual bool IsAlerting() const = 0;
  virtual const events::EventSet* GetEvents() const = 0;
  // Acknowledge all active events related to this data.
  virtual void Acknowledge() = 0;

  // Write item value.
  typedef std::function<void(const scada::Status&)> StatusCallback;
  virtual void Write(double value,
                     const scada::WriteFlags& flags,
                     const StatusCallback& callback) const = 0;

  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const StatusCallback& callback) const = 0;
};

}  // namespace rt