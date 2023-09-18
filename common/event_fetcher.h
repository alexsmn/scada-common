#pragma once

#include "base/containers/span.h"
#include "base/observer_list.h"
#include "common/event_set.h"
#include "common/node_event_provider.h"
#include "scada/history_service.h"

#include <deque>
#include <map>

namespace scada {
class HistoryService;
class MethodService;
class MonitoredItem;
class MonitoredItemService;
}  // namespace scada

class Executor;
class EventObserver;
class Logger;

struct EventFetcherContext {
  const std::shared_ptr<Executor> executor_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::HistoryService& history_service_;
  scada::MethodService& method_service_;
  const std::shared_ptr<const Logger> logger_;
};

class EventStorage {};

class EventAckQueue {};

// Fetches and provides unacked events, arranged by source nodes. Pulls unacked
// events from history and subscribes to updates via monitored item. Handles
// multiple event acknowledgement.
class EventFetcher : public NodeEventProvider, private EventFetcherContext {
 public:
  explicit EventFetcher(EventFetcherContext&& context);
  ~EventFetcher();

  void OnChannelOpened(const scada::NodeId& user_id);
  void OnChannelClosed();

  bool alarming() const { return alarming_; }

  void AcknowledgeAll();

  // NodeEventProvider
  virtual unsigned severity_min() const override { return severity_min_; }
  virtual void SetSeverityMin(unsigned severity) override;
  virtual const EventContainer& unacked_events() const override {
    return unacked_events_;
  }
  virtual const EventSet* GetItemUnackedEvents(
      const scada::NodeId& item_id) const override;
  virtual void AcknowledgeEvent(unsigned ack_id) override;
  virtual bool IsAcking() const override {
    return !pending_ack_event_ids_.empty() || !running_ack_event_ids_.empty();
  }
  virtual bool IsAlerting(const scada::NodeId& item_id) const override;
  virtual void AddObserver(EventObserver& observer) override {
    observers_.insert(&observer);
  }
  virtual void RemoveObserver(EventObserver& observer) override {
    observers_.erase(&observer);
  }
  virtual void AddItemObserver(const scada::NodeId& item_id,
                               EventObserver& observer) override {
    item_unacked_events_[item_id].observers.insert(&observer);
  }
  virtual void RemoveItemObserver(const scada::NodeId& item_id,
                                  EventObserver& observer) override {
    item_unacked_events_[item_id].observers.erase(&observer);
  }
  virtual void AcknowledgeItemEvents(const scada::NodeId& item_id) override;

 private:
  // TODO: Use signals.
  using ObserverSet = std::set<EventObserver*>;

  struct ItemEventData {
    EventSet events;
    ObserverSet observers;
  };

  // TODO: Consider using `unordered_map`.
  using ItemEventMap = std::map<scada::NodeId, ItemEventData>;

  const scada::Event* AddUnackedEvent(const scada::Event& event);
  EventContainer::node_type RemoveUnackedEvent(const scada::Event& event);
  void ClearUackedEvents();

  void UpdateAlarming();

  void ItemEventsChanged(const ObserverSet& observers,
                         const scada::NodeId& item_id,
                         const EventSet& events);

  void AckPendingEvents();
  void PostAckPendingEvents();

  void Update();

  void OnEvent(const scada::Event& event);
  void OnSystemEvents(base::span<const scada::Event> events);
  void OnHistoryReadEventsComplete(scada::Status&& status,
                                   std::vector<scada::Event>&& results);

  scada::NodeId user_id_;

  bool connected_ = false;

  std::shared_ptr<scada::MonitoredItem> monitored_item_;

  unsigned severity_min_ = scada::kSeverityMin;

  EventContainer unacked_events_;

  ItemEventMap item_unacked_events_;

  bool alarming_ = false;

  using EventIdQueue = std::deque<scada::EventAcknowledgeId>;
  EventIdQueue pending_ack_event_ids_;

  // Consider using `unordered_set`.
  using EventIdSet = std::set<scada::EventAcknowledgeId>;
  EventIdSet running_ack_event_ids_;

  ObserverSet observers_;

  bool ack_pending_ = false;

  base::WeakPtrFactory<EventFetcher> weak_factory_{this};
};

inline const EventSet* EventFetcher::GetItemUnackedEvents(
    const scada::NodeId& item_id) const {
  ItemEventMap::const_iterator i = item_unacked_events_.find(item_id);
  return i != item_unacked_events_.end() ? &i->second.events : nullptr;
}

inline bool EventFetcher::IsAlerting(const scada::NodeId& item_id) const {
  const EventSet* events = GetItemUnackedEvents(item_id);
  return events && !events->empty();
}
