#include "events/event_fetcher.h"

#include "base/awaitable.h"
#include "base/boost_log.h"
#include "base/check.h"
#include "base/range_util.h"
#include "events/event_ack_queue.h"
#include "events/event_observer.h"
#include "events/event_storage.h"
#include "model/devices_node_ids.h"
#include "scada/history_service.h"
#include "scada/monitored_item.h"
#include "scada/monitored_item_service.h"
#include "scada/monitoring_parameters.h"
#include "scada/read_value_id.h"
#include "scada/standard_node_ids.h"

#include <ranges>

#include "base/debug_util.h"

namespace {

// True for the device protocol-trace events, which the operator's event
// journal must not show.
//
// The subscription below asks for SystemEventType, and DeviceWatchEventType
// subtypes it (common/model/nodesets/devices.xml) — OPC UA event filters match
// subtypes (Part 4 §7.4.4,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/7.4.4), so every
// IEC-104 APDU an edge traces arrives here by design. There is no `OfType` an
// operator feed could ask for instead that would exclude them, so the exclusion
// has to happen on this side.
//
// The two surfaces are deliberately separate: the frame trace belongs to the
// device watch (the `deviceWatch` row in docs/parity/qt-web-parity.json), the
// journal to UC-5 process events. Left unfiltered, a link that is merely alive
// pushes every genuine event off the journal within seconds — measured
// 2026-08-22 against a standalone scada-iec104, where a single idle
// IEC-104 loopback link raised a few hundred trace events per minute, several
// of them 300-byte hex dumps.
//
// DeviceFrameEventType subtypes DeviceWatchEventType, so both are named: a
// frame is raised as scada::DeviceFrameEvent and so does not survive the
// any_cast in the subscription below today, but the history path assembles
// plain scada::Events, and a check that knew only one of the pair would let the
// other through.
//
// The ids are the canonical ones. That is correct on this side of the wire:
// a client session's services are wrapped by
// opcua_bridge::CreateRemappingClientDataServices, which translates the
// server's published namespace indexes back to canonical ones (ADR 0003
// phase 3a) before an event reaches here.
bool IsDeviceTraceEvent(const scada::Event& event) {
  return event.event_type_id == scada::devices::id::DeviceWatchEventType ||
         event.event_type_id == scada::devices::id::DeviceFrameEventType;
}

}  // namespace

EventFetcher::EventFetcher(EventFetcherContext&& context)
    : EventFetcherContext{std::move(context)},
      monitored_item_{monitored_item_adapter_.CreateMonitoredItem(
          scada::ReadValueId{scada::id::Server,
                             scada::AttributeId::EventNotifier},
          scada::MonitoringParameters{
              .filter = scada::EventFilter{
                  .of_type = {scada::id::SystemEventType}}})} {
  scada::base::Check(monitored_item_);

  monitored_item_->Subscribe(static_cast<scada::EventHandler>(
      [executor = executor_, cancelation = cancelation_.weak_ptr(), this](
          scada::Status status, std::any event) mutable {
        CoSpawn(
            executor, cancelation,
            [this, status = std::move(status),
             event = std::move(event)]() mutable -> Awaitable<void> {
              if (!status)
                co_return;

              if (event.has_value()) {
                // Events arrive from a (possibly remote) server; payloads of
                // an unexpected type are ignored.
                if (auto* system_event = std::any_cast<scada::Event>(&event)) {
                  OnSystemEvents({system_event, 1});
                }
              }
              co_return;
            });
      }));
}

EventFetcher::~EventFetcher() {
  cancelation_.Cancel();
}

const EventFetcher::EventContainer& EventFetcher::unacked_events() const {
  return event_storage_.events();
}

void EventFetcher::AddObserver(EventObserver& observer) {
  event_storage_.AddObserver(observer);
}

void EventFetcher::RemoveObserver(EventObserver& observer) {
  event_storage_.RemoveObserver(observer);
}

void EventFetcher::AddItemObserver(const scada::NodeId& node_id,
                                   EventObserver& observer) {
  event_storage_.AddNodeObserver(node_id, observer);
}

void EventFetcher::RemoveItemObserver(const scada::NodeId& node_id,
                                      EventObserver& observer) {
  event_storage_.RemoveNodeObserver(node_id, observer);
}

const EventSet* EventFetcher::GetItemUnackedEvents(
    const scada::NodeId& node_id) const {
  return event_storage_.GetNodeEvents(node_id);
}

bool EventFetcher::IsAlerting(const scada::NodeId& node_id) const {
  const EventSet* events = GetItemUnackedEvents(node_id);
  return events && !events->empty();
}

void EventFetcher::SetSeverityMin(scada::EventSeverity severity) {
  if (severity < scada::kSeverityMin || severity > scada::kSeverityMax)
    return;

  if (severity_min_ == severity)
    return;

  severity_min_ = severity;

  event_ack_queue_.Reset();

  event_storage_.Clear();

  Update();
}

void EventFetcher::OnSystemEvents(std::span<const scada::Event> events) {
  for (const auto& event : events) {
    if (event.acked) {
      event_ack_queue_.OnAcked(event.event_id);
    }
  }

  auto filtered_events =
      events |
      std::views::filter(
          [severity_min = severity_min_](const scada::Event& event) {
            return event.severity >= severity_min && !IsDeviceTraceEvent(event);
          }) |
      to_vector;

  if (!filtered_events.empty()) {
    event_storage_.Update(filtered_events);
  }
}

bool EventFetcher::IsAcking() const {
  return event_ack_queue_.IsAcking();
}

void EventFetcher::AcknowledgeItemEvents(const scada::NodeId& node_id) {
  const EventSet* events = GetItemUnackedEvents(node_id);
  if (!events)
    return;

  for (auto* event : *events) {
    AcknowledgeEvent(event->event_id);
  }
}

void EventFetcher::AcknowledgeEvent(scada::EventId ack_id) {
  event_ack_queue_.Ack(ack_id);
}

void EventFetcher::AcknowledgeAllEvents() {
  for (const auto& event : event_storage_.events() | std::views::values) {
    AcknowledgeEvent(event.event_id);
  }
}

void EventFetcher::Update() {
  CoSpawn(
      executor_, cancelation_,
      [this, cancelation = cancelation_.ref()]() mutable -> Awaitable<void> {
        auto result = co_await history_service_.HistoryReadEvents(
            scada::id::Server, {}, {},
            scada::EventFilter{scada::EventFilter::UNACKED});
        if (cancelation.canceled())
          co_return;
        OnHistoryReadEventsComplete(std::move(result));
      });
}

void EventFetcher::OnChannelOpened(const scada::ServiceContext& context) {
  connected_ = true;
  event_ack_queue_.OnChannelOpened(context);
  Update();
}

void EventFetcher::OnChannelClosed() {
  connected_ = false;
}

void EventFetcher::OnHistoryReadEventsComplete(
    scada::StatusOr<scada::HistoryReadEventsResult>&& result) {
  // A failed read is indistinguishable from an empty one here, as before: the
  // fetcher has no error surface and simply leaves the view unchanged.
  if (!result.ok())
    return;
  OnSystemEvents(std::move(result->events));
}
