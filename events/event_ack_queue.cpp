#include "events/event_ack_queue.h"

#include "base/any_executor_dispatch.h"
#include "base/awaitable.h"
#include "base/check.h"
#include "scada/method_service.h"
#include "scada/standard_node_ids.h"

#include <tuple>

EventAckQueue::EventAckQueue(EventAckQueueContext&& context)
    : EventAckQueueContext{std::move(context)} {}

EventAckQueue::~EventAckQueue() {
  cancelation_.Cancel();
}

void EventAckQueue::NotifyIfAckingChanged() {
  const bool acking = IsAcking();
  if (acking == reported_acking_)
    return;
  reported_acking_ = acking;
  if (acking_changed_handler_)
    acking_changed_handler_();
}

void EventAckQueue::OnAcked(scada::EventId acknowledge_id) {
  if (running_ack_event_ids_.erase(acknowledge_id)) {
    LOG_INFO(*logger_) << std::format("Event {} acknowledged", acknowledge_id);
    PostAckPendingEvents();
    NotifyIfAckingChanged();
  }
}

void EventAckQueue::PostAckPendingEvents() {
  if (running_ack_event_ids_.size() >= kMaxParallelAcks ||
      pending_ack_event_ids_.empty()) {
    scada::base::Check(!ack_pending_);
    return;
  }

  if (!ack_pending_) {
    ack_pending_ = true;
    Dispatch(executor_, cancelation_, [this] { AckPendingEvents(); });
  }
}

void EventAckQueue::AckPendingEvents() {
  scada::base::Check(ack_pending_);
  ack_pending_ = false;

  std::vector<scada::EventId> event_ids;
  while (running_ack_event_ids_.size() < kMaxParallelAcks &&
         !pending_ack_event_ids_.empty()) {
    auto ack_id = pending_ack_event_ids_.front();
    pending_ack_event_ids_.pop_front();

    event_ids.emplace_back(ack_id);
    running_ack_event_ids_.insert(ack_id);
  }

  if (!event_ids.empty() && foreign_ack_resolver_) {
    for (scada::EventId event_id : event_ids) {
      AckForeignEvent(event_id);
    }
  } else if (!event_ids.empty()) {
    LOG_INFO(*logger_) << std::format("Acknowledge events {}",
                                      scada::base::AsList(event_ids));
    CoSpawn(executor_, cancelation_,
            [this, event_ids = std::move(event_ids),
             context = service_context_]() mutable -> Awaitable<void> {
              // A success needs nothing here: the acknowledged events come back
              // through OnAcked. A refusal does not, so it has to be reported
              // from the call's own result or the operator never learns of it
              // (backlog 848).
              auto result = co_await method_service_.Call(
                  scada::id::Server,
                  scada::id::AcknowledgeableConditionType_Acknowledge,
                  {event_ids, scada::Now()}, std::move(context));
              if (!result.ok()) {
                OnAckFailed(event_ids, result.status());
              }
            });
  }

  PostAckPendingEvents();
}

void EventAckQueue::AckForeignEvent(scada::EventId event_id) {
  std::optional<ForeignAckTarget> target = foreign_ack_resolver_(event_id);
  if (!target || target->condition_id.is_null()) {
    // A plain event, or one the fetcher has already dropped: nothing to call.
    // Dispatched, so the handler never runs inside the operator's Ack.
    Dispatch(executor_, cancelation_, [this, event_id] {
      if (foreign_acked_handler_)
        foreign_acked_handler_(event_id);
      // The handler normally confirms through OnAcked; make sure the id is
      // released even if it did not.
      OnAcked(event_id);
    });
    return;
  }

  LOG_INFO(*logger_) << std::format("Acknowledge condition {} for event {}",
                                    target->condition_id.ToString(), event_id);
  CoSpawn(
      executor_, cancelation_,
      [this, event_id, target = std::move(*target),
       context = service_context_]() mutable -> Awaitable<void> {
        // Called on the Condition, never on `Server` and never on the
        // AcknowledgeableConditionType node, with the EventId exactly as
        // the server sent it and a Comment (OPC UA Part 9 §5.7.3
        // Acknowledge Method,
        // https://reference.opcfoundation.org/Core/Part9/v105/docs/5.7.3).
        // The client has no comment field yet, so the Comment is empty.
        auto result = co_await method_service_.Call(
            target.condition_id,
            scada::NodeId{scada::id::AcknowledgeableConditionType_Acknowledge},
            {scada::Variant{std::move(target.event_id)},
             scada::Variant{scada::LocalizedText{}}},
            std::move(context));
        if (!result.ok()) {
          const scada::EventId ids[] = {event_id};
          OnAckFailed(ids, result.status());
          co_return;
        }
        if (foreign_acked_handler_)
          foreign_acked_handler_(event_id);
        OnAcked(event_id);
      });
}

void EventAckQueue::OnAckFailed(std::span<const scada::EventId> event_ids,
                                const scada::Status& status) {
  LOG_WARNING(*logger_) << std::format(
      "Acknowledge of events {} refused: {}",
      scada::base::AsList(
          std::vector<scada::EventId>(event_ids.begin(), event_ids.end())),
      ToString(status));

  // Still running means no OnAcked has raced ahead. Releasing them lets the
  // operator retry; left in place they would block every later Ack of the
  // same events and keep IsAcking() true for good.
  for (scada::EventId event_id : event_ids) {
    running_ack_event_ids_.erase(event_id);
  }

  if (ack_failed_handler_) {
    ack_failed_handler_(event_ids, status);
  }

  PostAckPendingEvents();
  NotifyIfAckingChanged();
}

void EventAckQueue::Ack(scada::EventId ack_id) {
  if (running_ack_event_ids_.contains(ack_id) ||
      std::ranges::find(pending_ack_event_ids_, ack_id) !=
          pending_ack_event_ids_.end())
    return;

  pending_ack_event_ids_.push_back(ack_id);
  PostAckPendingEvents();
  NotifyIfAckingChanged();
}
