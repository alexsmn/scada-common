#include "common/event_manager.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logger.h"
#include "common/event_observer.h"
#include "core/event_service.h"
#include "core/history_service.h"
#include "core/monitored_item.h"
#include "core/monitored_item_service.h"
#include "core/standard_node_ids.h"

#include <boost/asio/io_context.hpp>

namespace events {

static const size_t kMaxParallelAcks = 5;

EventManager::EventManager(EventManagerContext&& context)
    : EventManagerContext{std::move(context)} {
  monitored_item_ = monitored_item_service_.CreateMonitoredItem(
      {scada::id::RootFolder, scada::AttributeId::EventNotifier});
  assert(monitored_item_);
  monitored_item_->set_event_handler(
      [this](const scada::Status& status, const scada::Event& event) {
        // TODO: Handle |status|
        OnEvent(event);
      });
  monitored_item_->Subscribe();
}

EventManager::~EventManager() {}

void EventManager::SetSeverityMin(unsigned severity) {
  if (severity_min_ == severity)
    return;

  severity_min_ = severity;

  ClearUackedEvents();

  for (ObserverSet::const_iterator i = observers_.begin();
       i != observers_.end();)
    (*i++)->OnAllEventsAcknowledged();

  Update();
}

void EventManager::OnEvent(const scada::Event& event) {
  if (event.acked)
    RemoveUnackedEvent(event);
  else
    AddUnackedEvent(event);
}

void EventManager::AddUnackedEvent(const scada::Event& event) {
  if (event.severity < severity_min_)
    return;

  std::pair<EventContainer::iterator, bool> p = unacked_events_.insert(
      EventContainer::value_type(event.acknowledge_id, event));
  scada::Event& contained_event = p.first->second;
  bool inserted = p.second;

  // Replace old event on update.
  if (!inserted) {
    // |tid| and |rid| mustn't be changed.
    assert(contained_event.node_id == event.node_id);
    // Replace existing event.
    contained_event = event;
  }

  // Notify observers about new event.
  for (ObserverSet::iterator i = observers_.begin(); i != observers_.end();) {
    EventObserver& observer = **i++;
    observer.OnEventReported(contained_event);
  }

  if (inserted && !event.node_id.is_null()) {
    ItemEventData& item_event_data = item_unacked_events_[event.node_id];
    item_event_data.events.insert(&contained_event);
    ItemEventsChanged(observers_, event.node_id, item_event_data.events);
    ItemEventsChanged(item_event_data.observers, event.node_id,
                      item_event_data.events);
  }

  UpdateAlarming();
}

void EventManager::RemoveUnackedEvent(const scada::Event& event) {
  EventContainer::iterator i = unacked_events_.find(event.acknowledge_id);
  if (i == unacked_events_.end())
    return;

  // Acknowledge confirmation.
  {
    EventIdSet::iterator i = running_ack_event_ids_.find(event.acknowledge_id);
    if (i != running_ack_event_ids_.end()) {
      running_ack_event_ids_.erase(i);
      logger_->WriteF(LogSeverity::Normal, "Event %d acknowledged",
                      event.acknowledge_id);
      PostAckPendingEvents();
    }
  }

  scada::Event& contained_event = i->second;
  // Update fields of contained event before notificaition to observers.
  contained_event = event;

  for (ObserverSet::iterator j = observers_.begin(); j != observers_.end();) {
    EventObserver& observer = **j++;
    observer.OnEventAcknowledged(contained_event);
  }

  if (!contained_event.node_id.is_null()) {
    ItemEventMap::iterator i =
        item_unacked_events_.find(contained_event.node_id);
    if (i != item_unacked_events_.end()) {
      ItemEventData& item_event_data = i->second;

      EventSet::iterator j = item_event_data.events.find(&contained_event);
      assert(j != item_event_data.events.end());
      item_event_data.events.erase(j);

      ItemEventsChanged(observers_, contained_event.node_id,
                        item_event_data.events);
      ItemEventsChanged(item_event_data.observers, contained_event.node_id,
                        item_event_data.events);

      if (item_event_data.events.empty() && item_event_data.observers.empty())
        item_unacked_events_.erase(i);
    }
  }

  unacked_events_.erase(i);

  UpdateAlarming();
}

void EventManager::ClearUackedEvents() {
  for (ItemEventMap::iterator i = item_unacked_events_.begin();
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

  running_ack_event_ids_.clear();
  pending_ack_event_ids_.clear();

  UpdateAlarming();
}

void EventManager::PostAckPendingEvents() {
  if (running_ack_event_ids_.size() >= kMaxParallelAcks ||
      pending_ack_event_ids_.empty()) {
    assert(!ack_pending_);
    return;
  }

  if (!ack_pending_) {
    ack_pending_ = true;
    io_context_.post([this] { AckPendingEvents(); });
  }
}

void EventManager::AckPendingEvents() {
  assert(ack_pending_);
  ack_pending_ = false;

  while (running_ack_event_ids_.size() < kMaxParallelAcks &&
         !pending_ack_event_ids_.empty()) {
    auto ack_id = pending_ack_event_ids_.front();
    pending_ack_event_ids_.pop_front();

    logger_->WriteF(LogSeverity::Normal, "Acknowledge event %u", ack_id);

    event_service_.Acknowledge(ack_id, user_id_);
    running_ack_event_ids_.insert(ack_id);
  }

  PostAckPendingEvents();
}

void EventManager::AcknowledgeItemEvents(const scada::NodeId& item_id) {
  const EventSet* events = GetItemUnackedEvents(item_id);
  if (!events)
    return;

  for (EventSet::const_iterator k = events->begin(); k != events->end(); ++k) {
    const scada::Event& event = **k;
    AcknowledgeEvent(event.acknowledge_id);
  }
}

void EventManager::AcknowledgeEvent(unsigned ack_id) {
  if (running_ack_event_ids_.find(ack_id) != running_ack_event_ids_.end())
    return;

  if (std::find(pending_ack_event_ids_.begin(), pending_ack_event_ids_.end(),
                ack_id) != pending_ack_event_ids_.end())
    return;

  pending_ack_event_ids_.push_back(ack_id);
  PostAckPendingEvents();
}

void EventManager::AcknowledgeAll() {
  for (EventContainer::const_iterator i = unacked_events_.begin();
       i != unacked_events_.end(); ++i) {
    AcknowledgeEvent(i->second.acknowledge_id);
  }
}

void EventManager::UpdateAlarming() {
  bool alarming = !unacked_events_.empty();

  if (alarming == alarming_)
    return;

  alarming_ = alarming;
}

void EventManager::ItemEventsChanged(const ObserverSet& observers,
                                     const scada::NodeId& item_id,
                                     const EventSet& events) {
  for (ObserverSet::const_iterator i = observers.begin(); i != observers.end();)
    (*i++)->OnItemEventsChanged(item_id, events);
}

void EventManager::Update() {
  history_service_.HistoryReadEvents(
      scada::id::RootFolder, {}, {}, {scada::Event::UNACKED},
      io_context_.wrap(
          [weak_ptr = weak_factory_.GetWeakPtr()](
              scada::Status status, std::vector<scada::Event> events) {
            if (auto* self = weak_ptr.get()) {
              self->OnHistoryReadEventsComplete(std::move(status),
                                                std::move(events));
            }
          }));
}

void EventManager::OnChannelOpened(const scada::NodeId& user_id) {
  connected_ = true;
  user_id_ = user_id;
  Update();
}

void EventManager::OnChannelClosed() {
  connected_ = false;
}

void EventManager::OnHistoryReadEventsComplete(
    scada::Status&& status,
    std::vector<scada::Event>&& events) {
  for (auto& event : events) {
    assert(!event.acked);
    OnEvent(event);
  }
}

}  // namespace events
