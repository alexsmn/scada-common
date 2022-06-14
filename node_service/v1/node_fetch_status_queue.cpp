#include "node_service/v1/node_fetch_status_queue.h"

#include "base/auto_reset.h"
#include "base/debug_util-inl.h"

namespace v1 {

// NodeFetchStatusQueue::ScopedStatusLock

NodeFetchStatusQueue::ScopedStatusLock::ScopedStatusLock(
    NodeFetchStatusQueue& queue)
    : queue_{queue} {
  assert(queue_.thread_checker_.CalledOnValidThread());
  ++queue_.status_lock_count_;
}

NodeFetchStatusQueue::ScopedStatusLock::~ScopedStatusLock() {
  assert(queue_.thread_checker_.CalledOnValidThread());
  assert(queue_.status_lock_count_ > 0);
  --queue_.status_lock_count_;
  if (queue_.status_lock_count_ == 0)
    queue_.NotifyPendingStatusChanged();
}

// NodeFetchStatusQueue

NodeFetchStatusQueue::NodeFetchStatusQueue(
    NodeFetchStatusChangedHandler node_fetch_status_changed_handler,
    NodeFetchStatusProvider node_fetch_status_provider)
    : node_fetch_status_changed_handler_{std::move(
          node_fetch_status_changed_handler)},
      node_fetch_status_provider_{std::move(node_fetch_status_provider)} {}

void NodeFetchStatusQueue::NotifyStatusChanged(const scada::NodeId& node_id) {
  LOG_INFO(logger_) << "Node status changed"
                    << LOG_TAG("NodeId", ToString(node_id));

  if (status_lock_count_ == 0) {
    auto node_status = node_fetch_status_provider_(node_id);
    NodeFetchStatusChangedItem item{node_id, std::move(node_status.first),
                                    node_status.second};
    node_fetch_status_changed_handler_(base::make_span(&item, 1));
    return;
  }

  pending_statuses_.emplace(node_id);
}

void NodeFetchStatusQueue::NotifyPendingStatusChanged() {
  assert(!notifying_);
  assert(status_lock_count_ >= 0);

  if (status_lock_count_ != 0)
    return;

  if (pending_statuses_.empty())
    return;

  base::AutoReset notifying{&notifying_, true};

  auto pending_statuses = std::move(pending_statuses_);
  pending_statuses_.clear();

  /*auto items = Map(pending_statuses, [this](const scada::NodeId& node_id) {
    auto [status, fetch_status] = GetStatus(node_id);
    return NodeFetchStatusChangedItem{node_id, std::move(status), fetch_status};
  });*/

  std::vector<NodeFetchStatusChangedItem> items;
  items.reserve(pending_statuses.size());
  for (const auto& node_id : pending_statuses) {
    auto node_status = node_fetch_status_provider_(node_id);
    items.emplace_back(NodeFetchStatusChangedItem{
        node_id, std::move(node_status.first), node_status.second});
  }

  node_fetch_status_changed_handler_(items);
}

void NodeFetchStatusQueue::CancelPendingStatus(const scada::NodeId& node_id) {
  pending_statuses_.erase(node_id);
}

}  // namespace v1
