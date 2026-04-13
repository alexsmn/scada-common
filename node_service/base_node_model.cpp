#include "node_service/base_node_model.h"

#include "base/auto_reset.h"
#include "node_service/node_observer.h"

BaseNodeModel::ScopedCallbackLock::ScopedCallbackLock(BaseNodeModel& model)
    : model_{model} {
  ++model_.callback_lock_count_;
}

BaseNodeModel::ScopedCallbackLock::~ScopedCallbackLock() {
  --model_.callback_lock_count_;
  if (model_.callback_lock_count_ == 0)
    model_.NotifyCallbacks();
}

void BaseNodeModel::Fetch(const NodeFetchStatus& requested_status,
                          const FetchCallback& callback) const {
  if (requested_status.all_less_or_equal(fetch_status_)) {
    if (!callback)
      return;
    // Route through the callback queue instead of firing inline so a
    // callback that calls Fetch() on this or another model stays off
    // the current stack frame. NotifyCallbacks() is re-entrance-safe
    // (see |notifying_callbacks_|) and drains iteratively.
    fetch_callbacks_.emplace_back(requested_status, callback);
    const_cast<BaseNodeModel*>(this)->NotifyCallbacks();

  } else {
    fetching_status_ |= requested_status;
    if (callback)
      fetch_callbacks_.emplace_back(requested_status, callback);
    // Must copy, since |fetching_status_| can be updated.
    auto combined_requested_status = fetching_status_;
    const_cast<BaseNodeModel*>(this)->OnFetchRequested(
        combined_requested_status);
  }
}

scada::Status BaseNodeModel::GetStatus() const {
  return status_;
}

NodeFetchStatus BaseNodeModel::GetFetchStatus() const {
  return fetch_status_;
}

void BaseNodeModel::Subscribe(NodeRefObserver& observer) const {
  assert(!observers_.HasObserver(&observer));
  observers_.AddObserver(&observer);
}

void BaseNodeModel::Unsubscribe(NodeRefObserver& observer) const {
  assert(observers_.HasObserver(&observer));
  observers_.RemoveObserver(&observer);
}

void BaseNodeModel::OnFetchRequested(const NodeFetchStatus& requested_status) {}

void BaseNodeModel::OnNodeDeleted() {
  SetFetchStatus(scada::StatusCode::Bad_WrongNodeId, NodeFetchStatus::Max());
}

void BaseNodeModel::SetFetchStatus(const scada::Status& status,
                                   const NodeFetchStatus& fetch_status) {
  status_ = status;

  if (fetch_status_ == fetch_status)
    return;

  fetch_status_ = fetch_status;
  fetching_status_ |= fetch_status;

  NotifyCallbacks();

  OnFetchStatusChanged();
}

void BaseNodeModel::NotifyCallbacks() {
  if (callback_lock_count_ != 0)
    return;

  // If a callback being invoked below calls Fetch() which calls back
  // into NotifyCallbacks, let the outermost frame own the drain —
  // nested calls return immediately and the outer while-loop picks
  // up whatever the callback enqueued.
  if (notifying_callbacks_)
    return;
  base::AutoReset<bool> drain_guard{&notifying_callbacks_, true};

  while (true) {
    std::vector<FetchCallback> callbacks;
    size_t p = 0;
    for (size_t i = 0; i < fetch_callbacks_.size(); ++i) {
      auto& c = fetch_callbacks_[i];
      assert(c.second);
      if (c.first.all_less_or_equal(fetch_status_))
        callbacks.emplace_back(std::move(c.second));
      else
        fetch_callbacks_[p++] = std::move(c);
    }
    fetch_callbacks_.erase(fetch_callbacks_.begin() + p,
                           fetch_callbacks_.end());

    if (fetch_callbacks_.empty())
      fetch_callbacks_.shrink_to_fit();

    if (callbacks.empty())
      break;

    for (auto& callback : callbacks)
      callback();
  }
}
