#include "events/event_fetcher.h"

#include "base/executor.h"
#include "base/logger.h"
#include "base/range_util.h"
#include "events/event_ack_queue.h"
#include "events/event_observer.h"
#include "events/event_storage.h"
#include "scada/history_service.h"
#include "scada/monitored_item.h"
#include "scada/monitored_item_service.h"
#include "scada/standard_node_ids.h"

#include <boost/range/adaptor/filtered.hpp>
#include <ranges>

#include "base/debug_util-inl.h"

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

        if (event.has_value()) {
          assert(std::any_cast<scada::Event>(&event));
          if (auto* system_event = std::any_cast<scada::Event>(&event))
            OnSystemEvents({system_event, 1});
        }
      })));
}

EventFetcher::~EventFetcher() = default;

const EventFetcher::EventContainer& EventFetcher::unacked_events() const {
  return event_storage_.events();
}

void EventFetcher::AddObserver(EventObserver& observer) {
  event_storage_.AddObserver(observer);
}

void EventFetcher::RemoveObserver(EventObserver& observer) {
  event_storage_.RemoveObserver(observer);
}

void EventFetcher::AddItemObserver(const scada::NodeId& item_id,
                                   EventObserver& observer) {
  event_storage_.AddNodeObserver(item_id, observer);
}

void EventFetcher::RemoveItemObserver(const scada::NodeId& item_id,
                                      EventObserver& observer) {
  event_storage_.RemoveNodeObserver(item_id, observer);
}

const EventSet* EventFetcher::GetItemUnackedEvents(
    const scada::NodeId& item_id) const {
  return event_storage_.GetNodeEvents(item_id);
}

bool EventFetcher::IsAlerting(const scada::NodeId& item_id) const {
  const EventSet* events = GetItemUnackedEvents(item_id);
  return events && !events->empty();
}

void EventFetcher::SetSeverityMin(unsigned severity) {
  if (severity < scada::kSeverityMin || severity > scada::kSeverityMax)
    return;

  if (severity_min_ == severity)
    return;

  severity_min_ = severity;

  event_ack_queue_.Reset();

  event_storage_.Clear();

  Update();
}

void EventFetcher::OnSystemEvents(base::span<const scada::Event> events) {
  for (const auto& event : events) {
    if (event.acked) {
      event_ack_queue_.OnAcked(event.acknowledge_id);
    }
  }

  auto filtered_events =
      events |
      boost::adaptors::filtered(
          [severity_min = severity_min_](const scada::Event& event) {
            return event.severity >= severity_min;
          }) |
      to_vector;

  if (!filtered_events.empty()) {
    event_storage_.Update(filtered_events);
  }
}

bool EventFetcher::IsAcking() const {
  return event_ack_queue_.IsAcking();
}

void EventFetcher::AcknowledgeItemEvents(const scada::NodeId& item_id) {
  const EventSet* events = GetItemUnackedEvents(item_id);
  if (!events)
    return;

  for (auto* event : *events) {
    AcknowledgeEvent(event->acknowledge_id);
  }
}

void EventFetcher::AcknowledgeEvent(unsigned ack_id) {
  event_ack_queue_.Ack(ack_id);
}

void EventFetcher::AcknowledgeAllEvents() {
  for (const auto& event : event_storage_.events() | std::views::values) {
    AcknowledgeEvent(event.acknowledge_id);
  }
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
  event_ack_queue_.OnChannelOpened(user_id);
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
