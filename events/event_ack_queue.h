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

  // Captures the acknowledging user's full service context (user id + rights) so
  // the acknowledge Call runs with the real caller's rights, letting the server
  // enforce the Call permission against them rather than a system identity.
  void OnChannelOpened(const scada::ServiceContext& context) {
    service_context_ = context;
  }

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
  // Releases the events of a refused call so they can be acknowledged again,
  // and reports the refusal.
  void OnAckFailed(std::span<const scada::EventId> event_ids,
                   const scada::Status& status);

  using EventIdQueue = std::deque<scada::EventId>;
  EventIdQueue pending_ack_event_ids_;

  // Consider using `unordered_set`.
  using EventIdSet = std::set<scada::EventId>;
  EventIdSet running_ack_event_ids_;

  bool ack_pending_ = false;

  // Acknowledging user's service context (user id + rights), captured when the
  // channel opens.
  scada::ServiceContext service_context_;

  Cancelation cancelation_;

  static const size_t kMaxParallelAcks = 5;
};
