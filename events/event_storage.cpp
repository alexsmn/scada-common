#include "events/event_storage.h"

#include "events/event_observer.h"

#include <algorithm>

#if defined(SCADA_USE_CORE_MODULE)
// Modules-pilot consumer (SCADA_CXX_MODULES=ON): base/scada names come from
// the scada.core facade. The import sits after the textual includes because
// the reverse order trips an AppleClang 21 declaration-merging bug in libc++.
import scada.core;
#else
#include "base/check.h"
#endif

const scada::Event* EventStorage::Add(const scada::Event& event) {
  auto [iter, inserted] = events_.try_emplace(event.event_id, event);
  scada::Event& contained_event = iter->second;

  // Replace the old event on update. A Condition's current event changes its
  // time and state as notifications arrive, so it leaves the time-ordered node
  // set before it changes and rejoins after.
  if (!inserted) {
    Unindex(contained_event);
    contained_event = event;
  }
  Index(contained_event);

  return &contained_event;
}

void EventStorage::Index(const scada::Event& event) {
  if (event.acked || event.source_node_id.is_null())
    return;
  NodeEntry& entry = node_events_[event.source_node_id];
  if (!entry.events.insert(&event).second)
    return;
  NodeEventsChanged(observers_, event.source_node_id, entry.events);
  NodeEventsChanged(entry.observers, event.source_node_id, entry.events);
}

void EventStorage::Unindex(const scada::Event& event) {
  if (event.source_node_id.is_null())
    return;
  auto p = node_events_.find(event.source_node_id);
  if (p == node_events_.end())
    return;
  NodeEntry& entry = p->second;
  if (entry.events.erase(&event) == 0)
    return;
  NodeEventsChanged(observers_, event.source_node_id, entry.events);
  NodeEventsChanged(entry.observers, event.source_node_id, entry.events);
  if (entry.events.empty() && entry.observers.empty())
    node_events_.erase(p);
}

EventStorage::EventContainer::node_type EventStorage::Remove(
    const scada::Event& event) {
  auto i = events_.find(event.event_id);
  if (i == events_.end())
    return {};

  scada::Event& contained_event = i->second;
  Unindex(contained_event);
  // Update fields of contained event before notification to observers.
  contained_event = event;

  return events_.extract(i);
}

void EventStorage::NotifyAckingChanged() {
  // Copied: an observer may remove itself from inside the notification.
  const ObserverSet observers = observers_;
  for (EventObserver* observer : observers)
    observer->OnAckingChanged();
}

void EventStorage::Clear() {
  for (auto i = node_events_.begin(); i != node_events_.end();) {
    auto& node_id = i->first;
    NodeEntry& entry = i->second;

    if (entry.observers.empty()) {
      node_events_.erase(i++);

    } else {
      entry.events.clear();
      NodeEventsChanged(observers_, node_id, entry.events);
      NodeEventsChanged(entry.observers, node_id, entry.events);
      ++i;
    }
  }

  events_.clear();

  UpdateAlerting();
}

void EventStorage::Update(std::span<const scada::Event> events) {
  // WARNING: Observers rely on stored event pointers. When event is deleted
  // from the storage it must be preserved while observers process the message.
  std::vector<EventContainer::node_type> deleted_nodes;

  std::vector<const scada::Event*> notify_events;

  for (auto& event : events) {
    if (!scada::IsRetained(event)) {
      if (auto node = Remove(event)) {
        // Observers must be notified with updates events, having `acked` flag
        // set.
        node.mapped() = event;
        notify_events.emplace_back(&node.mapped());
        deleted_nodes.emplace_back(std::move(node));
      }
    } else {
      if (auto* added_event = Add(event)) {
        notify_events.emplace_back(added_event);
      }
    }
  }

  UpdateAlerting();

  // Notify observers about new events.
  if (!notify_events.empty()) {
    for (auto i = observers_.begin(); i != observers_.end();) {
      EventObserver& observer = **i++;
      observer.OnEvents(notify_events);
    }
  }
}

// static
void EventStorage::NodeEventsChanged(const ObserverSet& observers,
                                     const scada::NodeId& node_id,
                                     const EventSet& events) {
  for (auto i = observers.begin(); i != observers.end();)
    (*i++)->OnItemEventsChanged(node_id, events);
}

void EventStorage::UpdateAlerting() {
  // Nothing left to acknowledge, though acknowledged Conditions may still
  // stand.
  bool alerting = std::ranges::any_of(
      events_, [](const auto& entry) { return !entry.second.acked; });

  if (alerting_ == alerting) {
    return;
  }

  alerting_ = alerting;

  for (auto i = observers_.begin(); i != observers_.end();) {
    (*i++)->OnAllEventsAcknowledged();
  }
}
