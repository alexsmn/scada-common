#pragma once

#include <span>

#include "base/memory/weak_ptr.h"
#include "events/node_event_provider.h"

namespace scada {
class HistoryService;
class MonitoredItem;
class MonitoredItemService;
struct HistoryReadEventsResult;
}  // namespace scada

class Executor;
class EventAckQueue;
class EventObserver;
class EventStorage;
class Logger;

struct EventFetcherContext {
  const std::shared_ptr<Executor> executor_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::HistoryService& history_service_;
  const std::shared_ptr<const Logger> logger_;
  EventStorage& event_storage_;
  EventAckQueue& event_ack_queue_;
};

// Fetches and provides unacked events, arranged by source nodes. Pulls unacked
// events from history and subscribes to updates via monitored item. Handles
// multiple event acknowledgement.
class EventFetcher : public NodeEventProvider, private EventFetcherContext {
 public:
  explicit EventFetcher(EventFetcherContext&& context);
  ~EventFetcher();

  void OnChannelOpened(const scada::NodeId& user_id);
  void OnChannelClosed();

  // NodeEventProvider
  virtual scada::EventSeverity severity_min() const override {
    return severity_min_;
  }
  virtual void SetSeverityMin(scada::EventSeverity severity) override;
  virtual const EventContainer& unacked_events() const;
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

  void OnSystemEvents(std::span<const scada::Event> events);
  void OnHistoryReadEventsComplete(scada::HistoryReadEventsResult&& result);

  bool connected_ = false;

  std::shared_ptr<scada::MonitoredItem> monitored_item_;

  scada::EventSeverity severity_min_ = scada::kSeverityMin;

  // Used to cancel the historical request.
  base::WeakPtrFactory<EventFetcher> weak_factory_{this};
};
