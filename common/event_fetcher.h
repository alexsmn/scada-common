#pragma once

#include "base/containers/span.h"
#include "base/memory/weak_ptr.h"
#include "common/event_set.h"
#include "common/event_storage.h"
#include "common/node_event_provider.h"
#include "scada/history_service.h"

namespace scada {
class HistoryService;
class MonitoredItem;
class MonitoredItemService;
}  // namespace scada

class Executor;
class EventAckQueue;
class EventObserver;
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

  void AcknowledgeAll();

  // NodeEventProvider
  virtual unsigned severity_min() const override { return severity_min_; }
  virtual void SetSeverityMin(unsigned severity) override;
  virtual const EventContainer& unacked_events() const override {
    return event_storage_.unacked_events();
  }
  virtual const EventSet* GetItemUnackedEvents(
      const scada::NodeId& item_id) const override;
  virtual void AcknowledgeEvent(unsigned ack_id) override;
  virtual bool IsAcking() const override;
  virtual bool IsAlerting(const scada::NodeId& item_id) const override;
  virtual void AddObserver(EventObserver& observer) override {
    event_storage_.AddObserver(observer);
  }
  virtual void RemoveObserver(EventObserver& observer) override {
    event_storage_.RemoveObserver(observer);
  }
  virtual void AddItemObserver(const scada::NodeId& item_id,
                               EventObserver& observer) override {
    event_storage_.AddItemObserver(item_id, observer);
  }
  virtual void RemoveItemObserver(const scada::NodeId& item_id,
                                  EventObserver& observer) override {
    event_storage_.RemoveItemObserver(item_id, observer);
  }
  virtual void AcknowledgeItemEvents(const scada::NodeId& item_id) override;

 private:
  void Update();

  void OnSystemEvents(base::span<const scada::Event> events);
  void OnHistoryReadEventsComplete(scada::Status&& status,
                                   std::vector<scada::Event>&& results);

  bool connected_ = false;

  std::shared_ptr<scada::MonitoredItem> monitored_item_;

  unsigned severity_min_ = scada::kSeverityMin;

  // Used to cancel the historical request.
  base::WeakPtrFactory<EventFetcher> weak_factory_{this};
};

inline const EventSet* EventFetcher::GetItemUnackedEvents(
    const scada::NodeId& item_id) const {
  return event_storage_.GetItemUnackedEvents(item_id);
}

inline bool EventFetcher::IsAlerting(const scada::NodeId& item_id) const {
  const EventSet* events = GetItemUnackedEvents(item_id);
  return events && !events->empty();
}
