#pragma once

#include "base/awaitable.h"
#include "base/logger.h"
#include "scada/event.h"

#include <algorithm>
#include <deque>
#include <memory>
#include <set>

namespace scada {
class CallbackToCoroutineMethodServiceAdapter;
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

  std::unique_ptr<scada::CallbackToCoroutineMethodServiceAdapter>
      method_service_adapter_;

  static const size_t kMaxParallelAcks = 5;
};
