#include "common/event_fetcher.h"

#include "base/executor.h"
#include "base/location.h"
#include "base/logger.h"
#include "common/event_observer.h"
#include "core/event_service.h"
#include "core/history_service.h"
#include "core/monitored_item.h"
#include "core/monitored_item_service.h"
#include "core/standard_node_ids.h"

#include "base/debug_util-inl.h"

static const size_t kMaxParallelAcks = 5;

EventFetcher::EventFetcher(EventFetcherContext&& context)
    : EventFetcherContext{std::move(context)} {
  monitored_item_ = monitored_item_service_.CreateMonitoredItem(
      scada::ReadValueId{scada::id::Server, scada::AttributeId::EventNotifier},
      scada::MonitoringParameters{}.set_filter(
          scada::EventFilter{}.set_of_type({scada::id::SystemEventType})));
  assert(monitored_item_);
  monitored_item_->Subscribe(static_cast<scada::EventHandler>(BindExecutor(
      executor_, [this](const scada::Status& status, const std::any& event) {
        // TODO: Handle |status|
        assert(status);
        assert(std::any_cast<scada::Event>(&event));
        if (auto* system_event = std::any_cast<scada::Event>(&event))
          OnSystemEvents({system_event, 1});
      })));
}

EventFetcher::~EventFetcher() {}

void EventFetcher::SetSeverityMin(unsigned severity) {
  if (severity_min_ == severity)
    return;

  severity_min_ = severity;

  ClearUackedEvents();

  for (ObserverSet::const_iterator i = observers_.begin();
       i != observers_.end();)
    (*i++)->OnAllEventsAcknowledged();

  Update();
}

void EventFetcher::OnSystemEvents(base::span<const scada::Event> events) {
  std::vector<EventContainer::node_type> deleted_nodes;
  std::vector<const scada::Event*> notify_events;
  for (auto& event : events) {
    if (event.acked) {
      auto node = RemoveUnackedEvent(event);
      notify_events.emplace_back(&node.mapped());
      deleted_nodes.emplace_back(std::move(node));
    } else {
      notify_events.emplace_back(AddUnackedEvent(event));
    }
  }

  // Notify observers about new event.
  if (!notify_events.empty()) {
    for (auto& event : notify_events) {
      for (auto i = observers_.begin(); i != observers_.end();) {
        EventObserver& observer = **i++;
        observer.OnEvent(*event);
      }
    }
  }
}

const scada::Event* EventFetcher::AddUnackedEvent(const scada::Event& event) {
  if (event.severity < severity_min_)
    return nullptr;

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

  if (inserted && !event.node_id.is_null()) {
    ItemEventData& item_event_data = item_unacked_events_[event.node_id];
    item_event_data.events.insert(&contained_event);
    ItemEventsChanged(observers_, event.node_id, item_event_data.events);
    ItemEventsChanged(item_event_data.observers, event.node_id,
                      item_event_data.events);
  }

  UpdateAlarming();

  return &contained_event;
}

EventFetcher::EventContainer::node_type EventFetcher::RemoveUnackedEvent(
    const scada::Event& event) {
  EventContainer::iterator i = unacked_events_.find(event.acknowledge_id);
  if (i == unacked_events_.end())
    return {};

  // Acknowledge confirmation.
  {
    auto j = running_ack_event_ids_.find(event.acknowledge_id);
    if (j != running_ack_event_ids_.end()) {
      running_ack_event_ids_.erase(j);
      logger_->WriteF(LogSeverity::Normal, "Event %d acknowledged",
                      event.acknowledge_id);
      PostAckPendingEvents();
    }
  }

  scada::Event& contained_event = i->second;
  // Update fields of contained event before notificaition to observers.
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

      if (item_event_data.events.empty() && item_event_data.observers.empty())
        item_unacked_events_.erase(p);
    }
  }

  auto node = unacked_events_.extract(i);

  UpdateAlarming();

  return node;
}

void EventFetcher::ClearUackedEvents() {
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

  running_ack_event_ids_.clear();
  pending_ack_event_ids_.clear();

  UpdateAlarming();
}

void EventFetcher::PostAckPendingEvents() {
  if (running_ack_event_ids_.size() >= kMaxParallelAcks ||
      pending_ack_event_ids_.empty()) {
    assert(!ack_pending_);
    return;
  }

  if (!ack_pending_) {
    ack_pending_ = true;
    Dispatch(*executor_, [this] { AckPendingEvents(); });
  }
}

void EventFetcher::AckPendingEvents() {
  assert(ack_pending_);
  ack_pending_ = false;

  std::vector<int> acknowledge_ids;
  while (running_ack_event_ids_.size() < kMaxParallelAcks &&
         !pending_ack_event_ids_.empty()) {
    auto ack_id = pending_ack_event_ids_.front();
    pending_ack_event_ids_.pop_front();

    acknowledge_ids.emplace_back(ack_id);
    running_ack_event_ids_.insert(ack_id);
  }

  if (!acknowledge_ids.empty()) {
    logger_->WriteF(LogSeverity::Normal, "Acknowledge events %s",
                    ToString(acknowledge_ids).c_str());
    event_service_.Acknowledge(acknowledge_ids, user_id_);
  }

  PostAckPendingEvents();
}

void EventFetcher::AcknowledgeItemEvents(const scada::NodeId& item_id) {
  const EventSet* events = GetItemUnackedEvents(item_id);
  if (!events)
    return;

  for (EventSet::const_iterator k = events->begin(); k != events->end(); ++k) {
    const scada::Event& event = **k;
    AcknowledgeEvent(event.acknowledge_id);
  }
}

void EventFetcher::AcknowledgeEvent(unsigned ack_id) {
  if (running_ack_event_ids_.find(ack_id) != running_ack_event_ids_.end())
    return;

  if (std::find(pending_ack_event_ids_.begin(), pending_ack_event_ids_.end(),
                ack_id) != pending_ack_event_ids_.end())
    return;

  pending_ack_event_ids_.push_back(ack_id);
  PostAckPendingEvents();
}

void EventFetcher::AcknowledgeAll() {
  for (EventContainer::const_iterator i = unacked_events_.begin();
       i != unacked_events_.end(); ++i) {
    AcknowledgeEvent(i->second.acknowledge_id);
  }
}

void EventFetcher::UpdateAlarming() {
  bool alarming = !unacked_events_.empty();

  if (alarming == alarming_)
    return;

  alarming_ = alarming;
}

void EventFetcher::ItemEventsChanged(const ObserverSet& observers,
                                     const scada::NodeId& item_id,
                                     const EventSet& events) {
  for (ObserverSet::const_iterator i = observers.begin(); i != observers.end();)
    (*i++)->OnItemEventsChanged(item_id, events);
}

void EventFetcher::Update() {
  history_service_.HistoryReadEvents(
      scada::id::Server, {}, {},
      scada::EventFilter{scada::EventFilter::UNACKED},
      BindExecutor(executor_,
                   [weak_ptr = weak_factory_.GetWeakPtr()](
                       scada::Status status, std::vector<scada::Event> events) {
                     if (auto* self = weak_ptr.get()) {
                       self->OnHistoryReadEventsComplete(std::move(status),
                                                         std::move(events));
                     }
                   }));
}

void EventFetcher::OnChannelOpened(const scada::NodeId& user_id) {
  connected_ = true;
  user_id_ = user_id;
  Update();
}

void EventFetcher::OnChannelClosed() {
  connected_ = false;
}

void EventFetcher::OnHistoryReadEventsComplete(
    scada::Status&& status,
    std::vector<scada::Event>&& events) {
  OnSystemEvents(events);
}
