#pragma once

#include "base/awaitable.h"
#include "base/any_executor_dispatch.h"
#include "scada/event.h"
#include "scada/method_service.h"
#include "scada/service_awaitable.h"

#include <algorithm>
#include <deque>
#include <set>

struct EventAckQueueContext {
  const std::shared_ptr<const Logger> logger_;
  AnyExecutor executor_;
  scada::MethodService& method_service_;
};

class EventAckQueue : private EventAckQueueContext {
 public:
  explicit EventAckQueue(EventAckQueueContext&& context)
      : EventAckQueueContext{std::move(context)} {}

  ~EventAckQueue() { cancelation_.Cancel(); }

  void OnChannelOpened(const scada::NodeId& user_id) { user_id_ = user_id; }

  bool IsAcking() const {
    return !pending_ack_event_ids_.empty() || !running_ack_event_ids_.empty();
  }

  void Ack(scada::EventId ack_id);

  // Acknowledge confirmation.
  void OnAcked(scada::EventId acknowledge_id);

  void Reset() {
    running_ack_event_ids_.clear();
    pending_ack_event_ids_.clear();
  }

 private:
  void AckPendingEvents();
  void PostAckPendingEvents();

  using EventIdQueue = std::deque<scada::EventId>;
  EventIdQueue pending_ack_event_ids_;

  // Consider using `unordered_set`.
  using EventIdSet = std::set<scada::EventId>;
  EventIdSet running_ack_event_ids_;

  bool ack_pending_ = false;

  // Acknowledger user ID.
  scada::NodeId user_id_;

  Cancelation cancelation_;

  static const size_t kMaxParallelAcks = 5;
};

inline void EventAckQueue::OnAcked(scada::EventId acknowledge_id) {
  if (running_ack_event_ids_.erase(acknowledge_id)) {
    logger_->WriteF(LogSeverity::Normal, "Event {} acknowledged",
                    acknowledge_id);
    PostAckPendingEvents();
  }
}

inline void EventAckQueue::PostAckPendingEvents() {
  if (running_ack_event_ids_.size() >= kMaxParallelAcks ||
      pending_ack_event_ids_.empty()) {
    assert(!ack_pending_);
    return;
  }

  if (!ack_pending_) {
    ack_pending_ = true;
    // TODO: Captures `this`.
    Dispatch(executor_, [this] { AckPendingEvents(); });
  }
}

inline void EventAckQueue::AckPendingEvents() {
  assert(ack_pending_);
  ack_pending_ = false;

  std::vector<scada::EventId> event_ids;
  while (running_ack_event_ids_.size() < kMaxParallelAcks &&
         !pending_ack_event_ids_.empty()) {
    auto ack_id = pending_ack_event_ids_.front();
    pending_ack_event_ids_.pop_front();

    event_ids.emplace_back(ack_id);
    running_ack_event_ids_.insert(ack_id);
  }

  if (!event_ids.empty()) {
    logger_->WriteF(LogSeverity::Normal, "Acknowledge events {}",
                    ToString(event_ids));
    CoSpawn(executor_, cancelation_,
            [executor = executor_, &method_service = method_service_,
             event_ids = std::move(event_ids),
             user_id = user_id_]() mutable -> Awaitable<void> {
              co_await scada::CallAsync(
                  executor, method_service, scada::id::Server,
                  scada::id::AcknowledgeableConditionType_Acknowledge,
                  {event_ids, scada::DateTime::Now()}, user_id);
            });
  }

  PostAckPendingEvents();
}

inline void EventAckQueue::Ack(scada::EventId ack_id) {
  if (running_ack_event_ids_.contains(ack_id) ||
      std::ranges::find(pending_ack_event_ids_, ack_id) !=
          pending_ack_event_ids_.end())
    return;

  pending_ack_event_ids_.push_back(ack_id);
  PostAckPendingEvents();
}
