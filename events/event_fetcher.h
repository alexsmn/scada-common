#pragma once

#include <any>
#include <map>
#include <optional>
#include <span>

#include "base/any_executor.h"
#include "base/boost_log.h"
#include "base/cancelation.h"
#include "events/event_ack_queue.h"
#include "events/node_event_provider.h"
#include "scada/event_filter.h"
#include "scada/legacy_monitored_item_adapter.h"
#include "scada/service_context.h"

namespace scada {
class HistoryService;
class MonitoredItem;
class MonitoredItemService;
struct HistoryReadEventsResult;
}  // namespace scada

class EventAckQueue;
class EventObserver;
class EventStorage;

struct EventFetcherContext {
  AnyExecutor executor_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::HistoryService& history_service_;
  const std::shared_ptr<BoostLogger> logger_;
  EventStorage& event_storage_;
  EventAckQueue& event_ack_queue_;
  // A third-party OPC UA server rather than ours (`scada::SessionService::
  // IsScada()` false, decided at sign-in by backlog 1054). Its events are asked
  // for by the standard fields, followed by ConditionId, and acknowledged per
  // Condition (backlog 1055).
  bool foreign_server_ = false;
};

// Fetches and provides unacked events, arranged by source nodes. Pulls unacked
// events from history and subscribes to updates via monitored item. Handles
// multiple event acknowledgement.
class EventFetcher : public NodeEventProvider, private EventFetcherContext {
 public:
  explicit EventFetcher(EventFetcherContext&& context);
  ~EventFetcher();

  void OnChannelOpened(const scada::ServiceContext& context);
  void OnChannelClosed();

  // NodeEventProvider
  virtual scada::EventSeverity severity_min() const override {
    return severity_min_;
  }
  virtual void SetSeverityMin(scada::EventSeverity severity) override;
  virtual const EventContainer& unacked_events() const override;
  virtual const EventSet* GetItemUnackedEvents(
      const scada::NodeId& node_id) const override;
  virtual void AcknowledgeEvent(scada::EventId ack_id) override;
  virtual bool IsAcking() const override;
  virtual bool IsAlerting(const scada::NodeId& node_id) const override;
  virtual void AddObserver(EventObserver& observer) override;
  virtual void RemoveObserver(EventObserver& observer) override;
  virtual void AddItemObserver(const scada::NodeId& node_id,
                               EventObserver& observer) override;
  virtual void RemoveItemObserver(const scada::NodeId& node_id,
                                  EventObserver& observer) override;
  virtual void AcknowledgeItemEvents(const scada::NodeId& node_id) override;
  virtual void AcknowledgeAllEvents() override;

 private:
  void Update();

  // (Re)creates the live event MonitoredItem on `Server`, asking for `fields`.
  void Subscribe(scada::EventFilter::Fields fields);
  void OnEventNotification(scada::Status status, std::any event);

  void OnSystemEvents(std::span<const scada::Event> events);

  // A third-party server's Condition notifications carry a new EventId for
  // every state change (OPC UA Part 9 §5.5.2 ConditionType); the journal keys
  // events by id. So every notification of one Condition is given the local id
  // its first one had, and keeps the wire EventId bytes for its Acknowledge.
  scada::Event CorrelateForeignCondition(scada::Event event);
  std::optional<ForeignAckTarget> ResolveForeignAckTarget(
      scada::EventId event_id) const;
  // Takes a third-party server's event out of the unacknowledged set once its
  // Acknowledge succeeded, without waiting for the server to say so.
  void OnForeignAcked(scada::EventId event_id);
  void OnHistoryReadEventsComplete(
      scada::StatusOr<scada::HistoryReadEventsResult>&& result);

  bool connected_ = false;

  // Bridges the service's subscription API to the single-item API used for the
  // system-events monitored item below.
  scada::LegacyMonitoredItemAdapter monitored_item_adapter_{
      executor_, monitored_item_service_};

  std::shared_ptr<scada::MonitoredItem> monitored_item_;
  // What `monitored_item_` asked for, and whether it has delivered an event.
  scada::EventFilter::Fields subscribed_fields_ =
      scada::EventFilter::Fields::kScada;
  bool received_event_ = false;

  // The local event id of each third-party Condition still unacknowledged.
  std::map<scada::NodeId, scada::EventId> condition_event_ids_;

  scada::EventSeverity severity_min_ = scada::kSeverityMin;

  // Used to cancel the historical request.
  Cancelation cancelation_;
};
