#include "common/event_storage.h"

#include "common/event_observer.h"

const scada::Event* EventStorage::AddUnackedEvent(const scada::Event& event) {
  auto [iter, inserted] =
      unacked_events_.try_emplace(event.acknowledge_id, event);
  scada::Event& contained_event = iter->second;

  // Replace old event on update.
  if (!inserted) {
    // |tid| and |rid| mustn't be changed.
    assert(contained_event.node_id == event.node_id);
    // Replace existing event.
    contained_event = event;
  }

  if (inserted && !event.node_id.is_null()) {
    ItemEventData& item_event_data = item_unacked_events_[event.node_id];
    item_event_data.events.insert(&contained_event);
    ItemEventsChanged(observers_, event.node_id, item_event_data.events);
    ItemEventsChanged(item_event_data.observers, event.node_id,
                      item_event_data.events);
  }

  UpdateAlerting();

  return &contained_event;
}

EventStorage::EventContainer::node_type EventStorage::RemoveUnackedEvent(
    const scada::Event& event) {
  auto i = unacked_events_.find(event.acknowledge_id);
  if (i == unacked_events_.end())
    return {};

  scada::Event& contained_event = i->second;
  // Update fields of contained event before notification to observers.
  contained_event = event;

  if (!contained_event.node_id.is_null()) {
    auto p = item_unacked_events_.find(contained_event.node_id);
    if (p != item_unacked_events_.end()) {
      ItemEventData& item_event_data = p->second;

      auto j = item_event_data.events.find(&contained_event);
      assert(j != item_event_data.events.end());
      item_event_data.events.erase(j);

      ItemEventsChanged(observers_, contained_event.node_id,
                        item_event_data.events);
      ItemEventsChanged(item_event_data.observers, contained_event.node_id,
                        item_event_data.events);

      if (item_event_data.events.empty() && item_event_data.observers.empty()) {
        item_unacked_events_.erase(p);
      }
    }
  }

  auto node = unacked_events_.extract(i);

  UpdateAlerting();

  return node;
}

void EventStorage::ClearUnackedEvents() {
  for (auto i = item_unacked_events_.begin();
       i != item_unacked_events_.end();) {
    auto& item_id = i->first;
    ItemEventData& data = i->second;

    if (data.observers.empty()) {
      item_unacked_events_.erase(i++);

    } else {
      data.events.clear();
      ItemEventsChanged(observers_, item_id, data.events);
      ItemEventsChanged(data.observers, item_id, data.events);
      ++i;
    }
  }

  unacked_events_.clear();
  UpdateAlerting();

  for (auto i = observers_.begin(); i != observers_.end();) {
    (*i++)->OnAllEventsAcknowledged();
  }
}

void EventStorage::OnSystemEvents(base::span<const scada::Event> events) {
  // WARNING: Observers rely on stored event pointers. When event is deleted
  // from the storage it must be preserved while observers process the message.
  std::vector<EventContainer::node_type> deleted_nodes;

  std::vector<const scada::Event*> notify_events;

  for (auto& event : events) {
    if (event.acked) {
      if (auto node = RemoveUnackedEvent(event)) {
        // Observers must be notified with updates events, having `acked` flag
        // set.
        node.mapped() = event;
        notify_events.emplace_back(&node.mapped());
        deleted_nodes.emplace_back(std::move(node));
      }
    } else {
      if (auto* added_event = AddUnackedEvent(event)) {
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
void EventStorage::ItemEventsChanged(const ObserverSet& observers,
                                     const scada::NodeId& item_id,
                                     const EventSet& events) {
  for (auto i = observers.begin(); i != observers.end();)
    (*i++)->OnItemEventsChanged(item_id, events);
}
