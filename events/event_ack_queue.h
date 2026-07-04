#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "base/logger.h"
#include "scada/event.h"
#include "scada/service_context.h"

#include <algorithm>
#include <deque>
#include <memory>
#include <set>

namespace scada {
class MethodService;
}  // namespace scada

struct EventAckQueueContext {
  const std::shared_ptr<const Logger> logger_;
  AnyExecutor executor_;
  scada::MethodService& method_service_;
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
