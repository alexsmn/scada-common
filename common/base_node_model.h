#pragma once

#include "base/observer_list.h"
#include "common/node_model.h"

class BaseNodeModel : public NodeModel {
 public:
  // NodeModel
  virtual void Fetch(const NodeFetchStatus& requested_status,
                     const FetchCallback& callback) const override;
  virtual NodeFetchStatus GetFetchStatus() const override;
  virtual scada::Status GetStatus() const override;
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;

 protected:
  explicit BaseNodeModel(scada::NodeId node_id);

  void SetFetchStatus(const scada::Status& status,
                      const NodeFetchStatus& fetch_status);

  virtual void OnFetchRequested(const NodeFetchStatus& requested_status);
  virtual void OnNodeDeleted();

  const scada::NodeId node_id_;

  scada::Status status_{scada::StatusCode::Good};

  NodeFetchStatus fetch_status_{};
  mutable std::vector<std::pair<NodeFetchStatus, FetchCallback>>
      fetch_callbacks_;

  mutable base::ObserverList<NodeRefObserver> observers_;
};
