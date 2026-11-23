#pragma once

#include "base/lifetime.h"
#include "events/event_set.h"
#include "events/node_event_provider.h"

#include <map>
#include <set>
#include <span>

// Stores the current events: every event still retained (`scada::IsRetained`)
// — the unacknowledged ones, and Conditions still active although
// acknowledged (backlog 1117). Observers subscribe for all updates, or per
// node for that node's UNACKNOWLEDGED events only: the per-node set is what
// makes an item flash, and an acknowledged alarm that is still standing does
// not flash (ISA-18.2's acknowledged-active state).
class EventStorage {
 public:
  using EventContainer = NodeEventProvider::EventContainer;

  const EventContainer& events() const SCADA_LIFETIME_BOUND { return events_; }

  // The node's unacknowledged events; null or empty when none.
  const EventSet* GetNodeEvents(const scada::NodeId& node_id) const
      SCADA_LIFETIME_BOUND {
    auto i = node_events_.find(node_id);
    return i != node_events_.end() ? &i->second.events : nullptr;
  }

  void Update(std::span<const scada::Event> events);
  void Clear();

  void AddObserver(EventObserver& observer) { observers_.insert(&observer); }
  void RemoveObserver(EventObserver& observer) { observers_.erase(&observer); }

  void AddNodeObserver(const scada::NodeId& node_id, EventObserver& observer) {
    node_events_[node_id].observers.insert(&observer);
  }

  void RemoveNodeObserver(const scada::NodeId& node_id,
                          EventObserver& observer) {
    node_events_[node_id].observers.erase(&observer);
  }

  // Whether any stored event is unacknowledged.
  bool alerting() const { return alerting_; }

  // Tells every observer added with AddObserver that the acknowledging state
  // changed. The storage does not track that state; the fetcher does.
  void NotifyAckingChanged();

 private:
  // TODO: Use signals.
  using ObserverSet = std::set<EventObserver*>;

  struct NodeEntry {
    EventSet events;
    ObserverSet observers;
  };

  const scada::Event* Add(const scada::Event& event) SCADA_LIFETIME_BOUND;
  EventContainer::node_type Remove(const scada::Event& event);

  // Adds `event`, already in `events_`, to its node's unacknowledged set when
  // it is unacknowledged; Unindex takes it out. The set orders by time, so an
  // event must be unindexed before any of its fields change.
  void Index(const scada::Event& event);
  void Unindex(const scada::Event& event);

  // This should be in an observer class.
  void UpdateAlerting();

  static void NodeEventsChanged(const ObserverSet& observers,
                                const scada::NodeId& node_id,
                                const EventSet& events);

  EventContainer events_;

  // TODO: Consider using `unordered_map`.
  std::map<scada::NodeId, NodeEntry> node_events_;

  ObserverSet observers_;

  bool alerting_ = false;
};
