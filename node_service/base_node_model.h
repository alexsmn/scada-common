#pragma once

#include "base/observer_list.h"
#include "node_service/node_model.h"

class BaseNodeModel : public NodeModel {
 public:
  virtual void OnNodeDeleted();

  // NodeModel
  virtual void Fetch(const NodeFetchStatus& requested_status,
                     const FetchCallback& callback) const override;
  virtual NodeFetchStatus GetFetchStatus() const override;
  virtual scada::Status GetStatus() const override;
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;

 protected:
  class ScopedCallbackLock {
   public:
    explicit ScopedCallbackLock(BaseNodeModel& model);
    ~ScopedCallbackLock();

   private:
    BaseNodeModel& model_;
  };

  void SetFetchStatus(const scada::Status& status,
                      const NodeFetchStatus& fetch_status);

  void NotifyCallbacks();

  virtual void OnFetchRequested(const NodeFetchStatus& requested_status);

  virtual void OnFetchStatusChanged() {}

  scada::Status status_{scada::StatusCode::Good};

  NodeFetchStatus fetch_status_{};

  int callback_lock_count_ = 0;
  mutable NodeFetchStatus fetching_status_{};
  mutable std::vector<std::pair<NodeFetchStatus, FetchCallback>>
      fetch_callbacks_;

  // Guards |NotifyCallbacks()| against synchronous re-entry. A
  // callback that calls Fetch() on this model (directly or via a
  // child) would otherwise run NotifyCallbacks nested in its own
  // stack frame; the outer loop below picks up anything the nested
  // callback enqueued without growing the stack.
  mutable bool notifying_callbacks_ = false;

  mutable base::ObserverList<NodeRefObserver> observers_;
};
