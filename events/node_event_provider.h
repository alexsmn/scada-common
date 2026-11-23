#pragma once

#include "base/lifetime.h"
#include "scada/event.h"

#include <map>

namespace scada {
class NodeId;
}

class EventObserver;
class EventSet;

class NodeEventProvider {
 public:
  virtual ~NodeEventProvider() = default;

  virtual scada::EventSeverity severity_min() const = 0;
  virtual void SetSeverityMin(scada::EventSeverity severity) = 0;

  using EventContainer = std::map<scada::EventId, scada::Event>;
  // The current alarm set: every event still retained (`scada::IsRetained`) —
  // the unacknowledged events, and Conditions still active although
  // acknowledged, each Condition as one row in its current state (backlog
  // 1117). An entry's `acked` is false while anything about it is unread;
  // `condition_id`/`active` say whether it is a Condition and whether it still
  // stands. Count unacknowledged entries by `acked`, never by the size.
  virtual const EventContainer& current_events() const SCADA_LIFETIME_BOUND = 0;

  // The item's unacknowledged current events (an acknowledged Condition still
  // standing does not flash); null or empty when none.
  virtual const EventSet* GetItemUnackedEvents(
      const scada::NodeId& item_id) const SCADA_LIFETIME_BOUND = 0;

  virtual void AcknowledgeEvent(scada::EventId) = 0;
  virtual void AcknowledgeItemEvents(const scada::NodeId& item_id) = 0;
  virtual void AcknowledgeAllEvents() = 0;

  virtual bool IsAcking() const = 0;
  virtual bool IsAlerting(const scada::NodeId& item_id) const = 0;

  virtual void AddObserver(EventObserver& observer) = 0;
  virtual void RemoveObserver(EventObserver& observer) = 0;

  virtual void AddItemObserver(const scada::NodeId& item_id,
                               EventObserver& observer) = 0;
  virtual void RemoveItemObserver(const scada::NodeId& item_id,
                                  EventObserver& observer) = 0;
};

// How many of `provider`'s current events still wait for an acknowledgement.
inline size_t CountUnackedEvents(const NodeEventProvider& provider) {
  size_t count = 0;
  for (const auto& entry : provider.current_events()) {
    if (!entry.second.acked)
      ++count;
  }
  return count;
}
