#pragma once

#include "base/time/time.h"
#include "node_service/node_ref.h"
#include "scada/data_value.h"
#include "scada/date_time_range.h"

#include <cassert>
#include <functional>
#include <span>

class EventSet;
class TimedDataObserver;
class TimedDataViewObserver;

extern const base::Time kTimedDataCurrentOnly;
extern const std::vector<scada::DateTimeRange> kReadyCurrentTimeOnly;

class TimedData {
 public:
  virtual ~TimedData() = default;

  virtual bool IsError() const { return false; }

  virtual const std::vector<scada::DateTimeRange>& GetReadyRanges() const = 0;

  virtual scada::DataValue GetDataValue() const = 0;
  virtual base::Time GetChangeTime() const = 0;

  // TODO: Replace with `std::span`.
  virtual std::span<const scada::DataValue> GetValues() const = 0;
  virtual scada::DataValue GetValueAt(const base::Time& time) const = 0;

  virtual void AddObserver(TimedDataObserver& observer) = 0;
  virtual void RemoveObserver(TimedDataObserver& observer) = 0;

  virtual void AddViewObserver(TimedDataViewObserver& observer,
                               const scada::DateTimeRange& range) = 0;
  virtual void RemoveViewObserver(TimedDataViewObserver& observer) = 0;

  virtual std::string GetFormula(bool aliases) const = 0;
  virtual scada::LocalizedText GetTitle() const = 0;
  virtual NodeRef GetNode() const = 0;

  virtual bool IsAlerting() const = 0;
  virtual const EventSet* GetEvents() const = 0;
  // Acknowledge all active events related to this data.
  virtual void Acknowledge() = 0;

  virtual std::string DumpDebugInfo() const = 0;
};
