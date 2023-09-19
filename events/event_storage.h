#pragma once

#include "events/event_set.h"
#include "events/node_event_provider.h"

#include <map>
#include <set>

// Stores unacked events. Provides are way for observers to subscribe for all
// updates or per node IDs.
class EventStorage {
 public:
  using EventContainer = NodeEventProvider::EventContainer;

  bool alerting() const { return alerting_; }

  const EventContainer& unacked_events() const { return unacked_events_; }

  const EventSet* GetItemUnackedEvents(const scada::NodeId& item_id) const {
    auto i = item_unacked_events_.find(item_id);
    return i != item_unacked_events_.end() ? &i->second.events : nullptr;
  }

  void AddObserver(EventObserver& observer) { observers_.insert(&observer); }

  void RemoveObserver(EventObserver& observer) { observers_.erase(&observer); }

  void AddItemObserver(const scada::NodeId& item_id, EventObserver& observer) {
    item_unacked_events_[item_id].observers.insert(&observer);
  }

  void RemoveItemObserver(const scada::NodeId& item_id,
                          EventObserver& observer) {
    item_unacked_events_[item_id].observers.erase(&observer);
  }

  void ClearUnackedEvents();

  void OnSystemEvents(base::span<const scada::Event> events);

 private:
  // TODO: Use signals.
  using ObserverSet = std::set<EventObserver*>;

  struct ItemEventData {
    EventSet events;
    ObserverSet observers;
  };

  // TODO: Consider using `unordered_map`.
  using ItemEventMap = std::map<scada::NodeId, ItemEventData>;

  const scada::Event* AddUnackedEvent(const scada::Event& event);
  EventContainer::node_type RemoveUnackedEvent(const scada::Event& event);

  // This should be in an observer class.
  void UpdateAlerting() { alerting_ = !unacked_events_.empty(); }

  static void ItemEventsChanged(const ObserverSet& observers,
                                const scada::NodeId& item_id,
                                const EventSet& events);

  EventContainer unacked_events_;

  ItemEventMap item_unacked_events_;

  ObserverSet observers_;

  bool alerting_ = false;
};
