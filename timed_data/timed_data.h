#pragma once

#include <cassert>
#include <functional>
#include <set>

#include "base/time/time.h"
#include "common/data_value_util.h"
#include "core/data_value.h"
#include "core/status.h"
#include "core/write_flags.h"
#include "node_service/node_ref.h"

class EventSet;
class TimedDataDelegate;

extern const base::Time kTimedDataCurrentOnly;
extern const std::vector<scada::DateTimeRange> kReadyCurrentTimeOnly;

class TimedData {
 public:
  virtual ~TimedData() = default;

  virtual bool IsError() const { return false; }

  virtual const std::vector<scada::DateTimeRange>& GetReadyRanges() const = 0;

  virtual scada::DataValue GetDataValue() const = 0;
  virtual base::Time GetChangeTime() const = 0;

  virtual const DataValues* GetValues() const = 0;
  virtual scada::DataValue GetValueAt(const base::Time& time) const = 0;

  virtual void AddObserver(TimedDataDelegate& observer,
                           const scada::DateTimeRange& range) = 0;
  virtual void RemoveObserver(TimedDataDelegate& observer) = 0;

  virtual std::string GetFormula(bool aliases) const = 0;
  virtual scada::LocalizedText GetTitle() const = 0;
  virtual NodeRef GetNode() const = 0;

  virtual bool IsAlerting() const = 0;
  virtual const EventSet* GetEvents() const = 0;
  // Acknowledge all active events related to this data.
  virtual void Acknowledge() = 0;

  // Write item value.
  using StatusCallback = std::function<void(const scada::Status&)>;
  virtual void Write(double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const StatusCallback& callback) const = 0;

  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback) const = 0;

  virtual std::string DumpDebugInfo() const = 0;
};
