#include "node_service/base_node_model.h"

#include "node_service/node_observer.h"

void BaseNodeModel::Fetch(const NodeFetchStatus& requested_status,
                          const FetchCallback& callback) const {
  if (status_ && requested_status <= fetch_status_) {
    if (callback)
      callback();

  } else {
    if (callback)
      fetch_callbacks_.emplace_back(requested_status, callback);
  }

  if (!status_ || fetching_status_ <= requested_status) {
    fetching_status_ |= requested_status;
    const_cast<BaseNodeModel*>(this)->OnFetchRequested(fetching_status_);
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

  std::vector<FetchCallback> callbacks;
  size_t p = 0;
  for (size_t i = 0; i < fetch_callbacks_.size(); ++i) {
    auto& c = fetch_callbacks_[i];
    assert(c.second);
    if (c.first <= fetch_status_)
      callbacks.emplace_back(std::move(c.second));
    else
      fetch_callbacks_[p++] = std::move(c);
  }
  fetch_callbacks_.erase(fetch_callbacks_.begin() + p, fetch_callbacks_.end());

  if (fetch_callbacks_.empty())
    fetch_callbacks_.shrink_to_fit();

  for (auto& callback : callbacks)
    callback();
}
