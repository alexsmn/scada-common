#include "events/event_fetcher.h"

#include "base/awaitable.h"
#include "base/boost_log.h"
#include "base/check.h"
#include "base/range_util.h"
#include "events/event_ack_queue.h"
#include "events/event_observer.h"
#include "events/event_storage.h"
#include "model/devices_node_ids.h"
#include "scada/event_id_codec.h"
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
// subtypes (Part 4 §7.7.3,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/7.7.3), so every
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
    : EventFetcherContext{std::move(context)} {
  event_ack_queue_.SetAckingChangedHandler(
      [this] { event_storage_.NotifyAckingChanged(); });

  if (foreign_server_) {
    event_ack_queue_.SetForeignAcknowledge(
        [this](scada::EventId event_id) {
          return ResolveForeignAckTarget(event_id);
        },
        [this](scada::EventId event_id) { OnForeignAcked(event_id); });
  }

  // Our server projects its own SCADA field list when the filter names none. A
  // third-party server is asked for the standard fields and the Condition
  // fields by name; one that does not know AcknowledgeableConditionType may
  // reject that whole filter, and is then asked for the standard fields alone
  // (OnEventNotification).
  Subscribe(foreign_server_
                ? scada::EventFilter::Fields::kStandardWithConditions
                : scada::EventFilter::Fields::kScada);
}

void EventFetcher::Subscribe(scada::EventFilter::Fields fields) {
  subscribed_fields_ = fields;
  received_event_ = false;
  // Our server raises every journal event as a SystemEventType subtype. A
  // third-party server's alarms are ConditionType subtypes, which an OfType
  // SystemEventType where clause would hide, so it is asked for every event.
  std::vector<scada::NodeId> of_type;
  if (!foreign_server_)
    of_type.emplace_back(scada::id::SystemEventType);
  monitored_item_ = monitored_item_adapter_.CreateMonitoredItem(
      scada::ReadValueId{scada::id::Server, scada::AttributeId::EventNotifier},
      scada::MonitoringParameters{
          .filter = scada::EventFilter{.of_type = std::move(of_type),
                                       .fields = fields}});
  scada::base::Check(monitored_item_);

  monitored_item_->Subscribe(static_cast<scada::EventHandler>(
      [executor = executor_, cancelation = cancelation_.weak_ptr(),
       item = std::weak_ptr<scada::MonitoredItem>{monitored_item_},
       this](scada::Status status, std::any event) mutable {
        CoSpawn(executor, cancelation,
                [this, item, status = std::move(status),
                 event = std::move(event)]() mutable -> Awaitable<void> {
                  // A notification of an item already replaced by a fallback.
                  if (item.lock() != monitored_item_)
                    co_return;
                  OnEventNotification(std::move(status), std::move(event));
                  co_return;
                });
      }));
}

void EventFetcher::OnEventNotification(scada::Status status, std::any event) {
  if (!status) {
    // open62541 refuses a whole event MonitoredItem when one select clause
    // names a type it does not have, though OPC UA Part 4 §7.22.3 EventFilter
    // (https://reference.opcfoundation.org/Core/Part4/v105/docs/7.22.3) wants
    // the error reported per clause. Without A&C it has no
    // AcknowledgeableConditionType, so ask again without the Condition fields.
    if (subscribed_fields_ ==
            scada::EventFilter::Fields::kStandardWithConditions &&
        !received_event_) {
      LOG_WARNING(*logger_) << std::format(
          "Event subscription with Condition fields refused ({}); "
          "subscribing to the standard event fields only",
          ToString(status));
      Subscribe(scada::EventFilter::Fields::kStandard);
    }
    return;
  }

  if (!event.has_value())
    return;

  // Events arrive from a (possibly remote) server; payloads of an unexpected
  // type are ignored.
  if (auto* system_event = std::any_cast<scada::Event>(&event)) {
    received_event_ = true;
    if (foreign_server_) {
      const scada::Event correlated =
          CorrelateForeignCondition(std::move(*system_event));
      OnSystemEvents({&correlated, 1});
    } else {
      OnSystemEvents({system_event, 1});
    }
  }
}

scada::Event EventFetcher::CorrelateForeignCondition(scada::Event event) {
  if (event.condition_id.is_null())
    return event;

  // The Acknowledge needs the EventId as it came off the wire. An id in our
  // own 8-byte layout was decoded rather than kept; put its bytes back before
  // the local id replaces it.
  if (event.foreign_event_id.empty())
    event.foreign_event_id = scada::EncodeEventIdByteString(event.event_id);

  auto [i, inserted] =
      condition_event_ids_.try_emplace(event.condition_id, event.event_id);
  event.event_id = i->second;
  // Acknowledged: it leaves the unacknowledged set, and a later activation of
  // the same Condition is a new entry.
  if (event.acked)
    condition_event_ids_.erase(i);
  return event;
}

std::optional<ForeignAckTarget> EventFetcher::ResolveForeignAckTarget(
    scada::EventId event_id) const {
  const auto& events = event_storage_.events();
  auto i = events.find(event_id);
  if (i == events.end())
    return std::nullopt;
  return ForeignAckTarget{.condition_id = i->second.condition_id,
                          .event_id = i->second.foreign_event_id};
}

void EventFetcher::OnForeignAcked(scada::EventId event_id) {
  const auto& events = event_storage_.events();
  auto i = events.find(event_id);
  if (i == events.end())
    return;
  scada::Event acked = i->second;
  acked.acked = true;
  acked.acknowledged_time = scada::Now();
  if (!acked.condition_id.is_null())
    condition_event_ids_.erase(acked.condition_id);
  OnSystemEvents({&acked, 1});
}

EventFetcher::~EventFetcher() {
  event_ack_queue_.SetAckingChangedHandler(nullptr);
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
  condition_event_ids_.clear();

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
  // The unacknowledged-events read is our server's: the filter it sends names
  // SCADA fields and an ACKED where clause no third-party server answers. A
  // third-party server's initial Condition state needs ConditionRefresh (OPC
  // UA Part 9 §5.5.7 ConditionRefresh Method,
  // https://reference.opcfoundation.org/Core/Part9/v105/docs/5.5.7), which is
  // not called yet (backlog 1055).
  if (foreign_server_)
    return;

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
