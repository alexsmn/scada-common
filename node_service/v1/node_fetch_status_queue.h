#pragma once

#include "base/boost_log.h"
#include "base/threading/thread_checker.h"
#include "scada/node_id.h"
#include "node_service/v1/node_fetch_status_types.h"

#include <set>

namespace v1 {

// Consolidates status notifications using locks.
// Takes actual statuses from |NodeFetchStatusProvider| and reports consolidates
// statuses to |NodeFetchStatusChangedHandler|.
class NodeFetchStatusQueue {
 public:
  using NodeStatus = std::pair<scada::Status, NodeFetchStatus>;

  using NodeFetchStatusProvider =
      std::function<NodeStatus(const scada::NodeId& node_id)>;

  explicit NodeFetchStatusQueue(
      NodeFetchStatusChangedHandler node_fetch_status_changed_handler,
      NodeFetchStatusProvider node_fetch_status_provider);

  void NotifyStatusChanged(const scada::NodeId& node_id);

  void NotifyPendingStatusChanged();

  void CancelPendingStatus(const scada::NodeId& node_id);

  class ScopedStatusLock {
   public:
    explicit ScopedStatusLock(NodeFetchStatusQueue& queue);
    ~ScopedStatusLock();

    ScopedStatusLock(const ScopedStatusLock&) = delete;
    ScopedStatusLock& operator=(const ScopedStatusLock&) = delete;

   private:
    NodeFetchStatusQueue& queue_;
  };

  ScopedStatusLock Lock() { return ScopedStatusLock{*this}; }

 private:
  const NodeFetchStatusChangedHandler node_fetch_status_changed_handler_;
  const NodeFetchStatusProvider node_fetch_status_provider_;

  BoostLogger logger_{LOG_NAME("NodeFetchStatusQueue")};

  base::ThreadChecker thread_checker_;

  std::set<scada::NodeId> pending_statuses_;
  bool notifying_ = false;
  int status_lock_count_ = 0;
};

}  // namespace v1
