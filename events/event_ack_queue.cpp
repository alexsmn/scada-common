#include "events/event_ack_queue.h"

#include "base/any_executor_dispatch.h"
#include "base/awaitable.h"
#include "scada/coroutine_services.h"
#include "scada/method_service.h"
#include "scada/standard_node_ids.h"

EventAckQueue::EventAckQueue(EventAckQueueContext&& context)
    : EventAckQueueContext{std::move(context)},
      method_service_adapter_{
          std::make_unique<scada::CallbackToCoroutineMethodServiceAdapter>(
              executor_, method_service_)} {}

EventAckQueue::~EventAckQueue() {
  cancelation_.Cancel();
}

void EventAckQueue::OnAcked(scada::EventId acknowledge_id) {
  if (running_ack_event_ids_.erase(acknowledge_id)) {
    logger_->WriteF(LogSeverity::Normal, "Event {} acknowledged",
                    acknowledge_id);
    PostAckPendingEvents();
  }
}

void EventAckQueue::PostAckPendingEvents() {
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

void EventAckQueue::AckPendingEvents() {
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
            [this, event_ids = std::move(event_ids),
             user_id = user_id_]() mutable -> Awaitable<void> {
              co_await method_service_adapter_->Call(
                  scada::id::Server,
                  scada::id::AcknowledgeableConditionType_Acknowledge,
                  {event_ids, scada::DateTime::Now()}, user_id);
            });
  }

  PostAckPendingEvents();
}

void EventAckQueue::Ack(scada::EventId ack_id) {
  if (running_ack_event_ids_.contains(ack_id) ||
      std::ranges::find(pending_ack_event_ids_, ack_id) !=
          pending_ack_event_ids_.end())
    return;

  pending_ack_event_ids_.push_back(ack_id);
  PostAckPendingEvents();
}
