#pragma once

#include "base/observer_list.h"
#include "common/event_set.h"
#include "core/history_service.h"

#include <deque>
#include <map>

class Logger;

namespace boost::asio {
class io_context;
}

namespace scada {
class EventService;
class HistoryService;
class MonitoredItem;
class MonitoredItemService;
}  // namespace scada

namespace events {

class EventObserver;

struct EventManagerContext {
  boost::asio::io_context& io_context_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::EventService& event_service_;
  scada::HistoryService& history_service_;
  const std::shared_ptr<const Logger> logger_;
};

// Holds new events, arranged by objects.
// Handles multiple event acknowledgement.
class EventManager : private EventManagerContext {
 public:
  typedef std::map<unsigned, scada::Event> EventContainer;

  explicit EventManager(EventManagerContext&& context);
  ~EventManager();

  void OnChannelOpened(const scada::NodeId& user_id);
  void OnChannelClosed();

  unsigned severity_min() const { return severity_min_; }
  void SetSeverityMin(unsigned severity);

  const EventContainer& unacked_events() const { return unacked_events_; }

  bool alarming() const { return alarming_; }
  bool is_acking() const {
    return !pending_ack_event_ids_.empty() || !running_ack_event_ids_.empty();
  }

  const EventSet* GetItemUnackedEvents(const scada::NodeId& item_id) const;
  bool IsAlerting(const scada::NodeId& item_id) const;

  void AcknowledgeItemEvents(const scada::NodeId& item_id);
  void AcknowledgeEvent(unsigned ack_id);
  void AcknowledgeAll();

  void AddObserver(EventObserver& observer) { observers_.insert(&observer); }
  void RemoveObserver(EventObserver& observer) { observers_.erase(&observer); }
  void AddItemObserver(const scada::NodeId& item_id, EventObserver& observer) {
    item_unacked_events_[item_id].observers.insert(&observer);
  }
  void RemoveItemObserver(const scada::NodeId& item_id,
                          EventObserver& observer) {
    item_unacked_events_[item_id].observers.erase(&observer);
  }

 protected:
  // EventSource::Delegate
  virtual void OnEvent(const scada::Event& event);

 private:
  typedef std::set<EventObserver*> ObserverSet;

  struct ItemEventData {
    EventSet events;
    ObserverSet observers;
  };

  typedef std::map<scada::NodeId, ItemEventData> ItemEventMap;

  void AddUnackedEvent(const scada::Event& event);
  void RemoveUnackedEvent(const scada::Event& event);
  void ClearUackedEvents();

  void UpdateAlarming();

  void ItemEventsChanged(const ObserverSet& observers,
                         const scada::NodeId& item_id,
                         const EventSet& events);

  void AckPendingEvents();
  void PostAckPendingEvents();

  void Update();

  void OnHistoryReadEventsComplete(scada::Status&& status,
                                   std::vector<scada::Event>&& results);

  scada::NodeId user_id_;

  bool connected_ = false;

  std::unique_ptr<scada::MonitoredItem> monitored_item_;

  unsigned severity_min_ = scada::kSeverityMin;

  EventContainer unacked_events_;

  ItemEventMap item_unacked_events_;

  bool alarming_ = false;

  typedef std::deque<scada::EventAcknowledgeId> EventIdQueue;
  EventIdQueue pending_ack_event_ids_;

  typedef std::set<scada::EventAcknowledgeId> EventIdSet;
  EventIdSet running_ack_event_ids_;

  ObserverSet observers_;

  bool ack_pending_ = false;

  base::WeakPtrFactory<EventManager> weak_factory_{this};
};

inline const EventSet* EventManager::GetItemUnackedEvents(
    const scada::NodeId& item_id) const {
  ItemEventMap::const_iterator i = item_unacked_events_.find(item_id);
  return i != item_unacked_events_.end() ? &i->second.events : nullptr;
}

inline bool EventManager::IsAlerting(const scada::NodeId& item_id) const {
  const EventSet* events = GetItemUnackedEvents(item_id);
  return events && !events->empty();
}

}  // namespace events
