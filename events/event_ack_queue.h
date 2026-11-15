#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "base/boost_log.h"
#include "scada/event.h"
#include "scada/service_context.h"

#include <algorithm>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <span>

namespace scada {
class MethodService;
}  // namespace scada

// Called when the Server refuses an acknowledgement, with the events the
// refused call carried and the status it answered. Runs on the queue's
// executor.
using EventAckFailedHandler =
    std::function<void(std::span<const scada::EventId> event_ids,
                       const scada::Status& status)>;

// Where a third-party server's event is acknowledged: the Condition the
// Acknowledge method is called on, and the EventId bytes of its latest state
// notification (OPC UA Part 9 §5.7.3 Acknowledge Method,
// https://reference.opcfoundation.org/Core/Part9/v105/docs/5.7.3). A null
// `condition_id` is a plain event, which has nothing to acknowledge on the
// server and is acknowledged on this side only.
struct ForeignAckTarget {
  scada::NodeId condition_id;
  scada::ByteString event_id;
};

// Resolves a local event id to its ForeignAckTarget; nullopt for an event the
// fetcher no longer holds.
using ForeignAckResolver =
    std::function<std::optional<ForeignAckTarget>(scada::EventId event_id)>;

// Called once a third-party server's event is acknowledged: its Acknowledge
// call succeeded, or it was a plain event. Runs on the queue's executor.
using ForeignAckedHandler = std::function<void(scada::EventId event_id)>;

struct EventAckQueueContext {
  const std::shared_ptr<BoostLogger> logger_;
  AnyExecutor executor_;
  scada::MethodService& method_service_;
  // Optional. Without it a refusal is only logged.
  EventAckFailedHandler ack_failed_handler_ = {};
};

class EventAckQueue : private EventAckQueueContext {
 public:
  explicit EventAckQueue(EventAckQueueContext&& context);

  ~EventAckQueue();

  // Captures the acknowledging user's full service context (user id + rights)
  // so the acknowledge Call runs with the real caller's rights, letting the
  // server enforce the Call permission against them rather than a system
  // identity.
  void OnChannelOpened(const scada::ServiceContext& context) {
    service_context_ = context;
  }

  bool IsAcking() const {
    return !pending_ack_event_ids_.empty() || !running_ack_event_ids_.empty();
  }

  // Called after IsAcking() changes value, from whichever call changed it.
  // One handler; setting another replaces it, and an empty one clears it.
  void SetAckingChangedHandler(std::function<void()> handler) {
    acking_changed_handler_ = std::move(handler);
  }

  // Switches the queue to a third-party server's acknowledgement: one Part 9
  // Acknowledge call per event, on its Condition, instead of our server's batch
  // call on `Server`. The fetcher sets it before the first Ack.
  void SetForeignAcknowledge(ForeignAckResolver resolver,
                             ForeignAckedHandler acked_handler) {
    foreign_ack_resolver_ = std::move(resolver);
    foreign_acked_handler_ = std::move(acked_handler);
  }

  void Ack(scada::EventId ack_id);

  // Acknowledge confirmation.
  void OnAcked(scada::EventId acknowledge_id);

  void Reset() {
    running_ack_event_ids_.clear();
    pending_ack_event_ids_.clear();
    NotifyIfAckingChanged();
  }

 private:
  void AckPendingEvents();
  void PostAckPendingEvents();
  // Acknowledges one third-party server's event (see SetForeignAcknowledge).
  void AckForeignEvent(scada::EventId event_id);
  // Releases the events of a refused call so they can be acknowledged again,
  // and reports the refusal.
  void OnAckFailed(std::span<const scada::EventId> event_ids,
                   const scada::Status& status);

  // Compares IsAcking() with the value last reported and calls the handler
  // when they differ.
  void NotifyIfAckingChanged();

  using EventIdQueue = std::deque<scada::EventId>;
  EventIdQueue pending_ack_event_ids_;

  // Consider using `unordered_set`.
  using EventIdSet = std::set<scada::EventId>;
  EventIdSet running_ack_event_ids_;

  bool ack_pending_ = false;

  bool reported_acking_ = false;
  std::function<void()> acking_changed_handler_;

  // Acknowledging user's service context (user id + rights), captured when the
  // channel opens.
  scada::ServiceContext service_context_;

  ForeignAckResolver foreign_ack_resolver_;
  ForeignAckedHandler foreign_acked_handler_;

  Cancelation cancelation_;

  static const size_t kMaxParallelAcks = 5;
};
