#pragma once

#include "base/containers/contains.h"
#include "base/executor.h"
#include "scada/event.h"
#include "scada/method_service.h"

#include <deque>
#include <set>

struct EventAckQueueContext {
  const std::shared_ptr<const Logger> logger_;
  const std::shared_ptr<Executor> executor_;
  scada::MethodService& method_service_;
};

class EventAckQueue : private EventAckQueueContext {
 public:
  explicit EventAckQueue(EventAckQueueContext&& context)
      : EventAckQueueContext{std::move(context)} {}

  void OnChannelOpened(const scada::NodeId& user_id) { user_id_ = user_id; }

  bool IsAcking() const {
    return !pending_ack_event_ids_.empty() || !running_ack_event_ids_.empty();
  }

  void Ack(scada::EventAcknowledgeId ack_id);

  // Acknowledge confirmation.
  void OnAcked(scada::EventAcknowledgeId acknowledge_id);

  void Reset() {
    running_ack_event_ids_.clear();
    pending_ack_event_ids_.clear();
  }

 private:
  void AckPendingEvents();
  void PostAckPendingEvents();

  using EventIdQueue = std::deque<scada::EventAcknowledgeId>;
  EventIdQueue pending_ack_event_ids_;

  // Consider using `unordered_set`.
  using EventIdSet = std::set<scada::EventAcknowledgeId>;
  EventIdSet running_ack_event_ids_;

  bool ack_pending_ = false;

  // Acknowledger user ID.
  scada::NodeId user_id_;

  static const size_t kMaxParallelAcks = 5;
};

inline void EventAckQueue::OnAcked(scada::EventAcknowledgeId acknowledge_id) {
  if (running_ack_event_ids_.erase(acknowledge_id)) {
    logger_->WriteF(LogSeverity::Normal, "Event %d acknowledged",
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
    Dispatch(*executor_, [this] { AckPendingEvents(); });
  }
}

inline void EventAckQueue::AckPendingEvents() {
  assert(ack_pending_);
  ack_pending_ = false;

  std::vector<scada::EventAcknowledgeId> acknowledge_ids;
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
    method_service_.Call(scada::id::Server,
                         scada::id::AcknowledgeableConditionType_Acknowledge,
                         {acknowledge_ids, scada::DateTime::Now()}, user_id_,
                         [](scada::Status&& status) {});
  }

  PostAckPendingEvents();
}

inline void EventAckQueue::Ack(scada::EventAcknowledgeId ack_id) {
  if (base::Contains(running_ack_event_ids_, ack_id) ||
      base::Contains(pending_ack_event_ids_, ack_id))
    return;

  pending_ack_event_ids_.push_back(ack_id);
  PostAckPendingEvents();
}
