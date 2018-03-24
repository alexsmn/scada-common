#include "common/base_node_model.h"

#include "common/node_observer.h"

BaseNodeModel::BaseNodeModel(scada::NodeId node_id)
    : node_id_{std::move(node_id)} {}

void BaseNodeModel::Fetch(const NodeFetchStatus& requested_status,
                          const FetchCallback& callback) const {
  if (requested_status <= fetch_status_) {
    if (callback)
      callback();

  } else {
    if (callback)
      fetch_callbacks_.emplace_back(requested_status, callback);

    const_cast<BaseNodeModel*>(this)->OnFetch(requested_status);
  }
}

scada::Status BaseNodeModel::GetStatus() const {
  return scada::StatusCode::Good;
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

void BaseNodeModel::OnFetch(const NodeFetchStatus& requested_status) {
}

void BaseNodeModel::OnNodeDeleted() {
  if (fetch_status_ == NodeFetchStatus::Max())
    return;

  fetch_status_ = NodeFetchStatus::Max();

  std::vector<FetchCallback> callbacks;
  callbacks.reserve(fetch_callbacks_.size());
  for (auto& c : fetch_callbacks_)
    callbacks.emplace_back(std::move(c.second));

  fetch_callbacks_.clear();
  fetch_callbacks_.shrink_to_fit();

  for (auto& callback : callbacks)
    callback();
}

void BaseNodeModel::SetFetchStatus(const NodeFetchStatus& status) {
  fetch_status_ = status;

  std::vector<FetchCallback> callbacks;
  size_t p = 0;
  for (size_t i = 0; i < fetch_callbacks_.size(); ++i) {
    auto& c = fetch_callbacks_[i];
    assert(c.second);
    if (c.first <= status)
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
