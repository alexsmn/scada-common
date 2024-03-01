#pragma once

#include "events/event_set.h"
#include "events/node_event_provider.h"

#include <map>
#include <set>
#include <span>

// Stores unacked events. Provides are way for observers to subscribe for all
// updates or per node IDs.
class EventStorage {
 public:
  using EventContainer = NodeEventProvider::EventContainer;

  const EventContainer& events() const { return events_; }

  const EventSet* GetNodeEvents(const scada::NodeId& node_id) const {
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

  bool alerting() const { return alerting_; }

 private:
  // TODO: Use signals.
  using ObserverSet = std::set<EventObserver*>;

  struct NodeEntry {
    EventSet events;
    ObserverSet observers;
  };

  const scada::Event* Add(const scada::Event& event);
  EventContainer::node_type Remove(const scada::Event& event);

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
