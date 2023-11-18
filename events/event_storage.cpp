#include "events/event_storage.h"

#include "events/event_observer.h"

const scada::Event* EventStorage::Add(const scada::Event& event) {
  auto [iter, inserted] = events_.try_emplace(event.event_id, event);
  scada::Event& contained_event = iter->second;

  // Replace old event on update.
  if (!inserted) {
    // |tid| and |rid| mustn't be changed.
    assert(contained_event.node_id == event.node_id);
    // Replace existing event.
    contained_event = event;
  }

  if (inserted && !event.node_id.is_null()) {
    NodeEntry& entry = node_events_[event.node_id];
    entry.events.insert(&contained_event);
    NodeEventsChanged(observers_, event.node_id, entry.events);
    NodeEventsChanged(entry.observers, event.node_id, entry.events);
  }

  UpdateAlerting();

  return &contained_event;
}

EventStorage::EventContainer::node_type EventStorage::Remove(
    const scada::Event& event) {
  auto i = events_.find(event.event_id);
  if (i == events_.end())
    return {};

  scada::Event& contained_event = i->second;
  // Update fields of contained event before notification to observers.
  contained_event = event;

  if (!contained_event.node_id.is_null()) {
    auto p = node_events_.find(contained_event.node_id);
    if (p != node_events_.end()) {
      NodeEntry& entry = p->second;

      auto j = entry.events.find(&contained_event);
      assert(j != entry.events.end());
      entry.events.erase(j);

      NodeEventsChanged(observers_, contained_event.node_id, entry.events);
      NodeEventsChanged(entry.observers, contained_event.node_id, entry.events);

      if (entry.events.empty() && entry.observers.empty()) {
        node_events_.erase(p);
      }
    }
  }

  auto node = events_.extract(i);

  UpdateAlerting();

  return node;
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
    if (event.acked) {
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
  bool alerting = !events_.empty();

  if (alerting_ == alerting) {
    return;
  }

  alerting_ = alerting;

  for (auto i = observers_.begin(); i != observers_.end();) {
    (*i++)->OnAllEventsAcknowledged();
  }
}
