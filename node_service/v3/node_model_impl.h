#pragma once

#include "base/awaitable.h"
#include "base/lifetime.h"
#include "common/node_state.h"
#include "node_service/node_events.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_ref.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace scada {
struct ModelChangeEvent;
}

namespace v3 {

class NodeModelImpl;
class NodeServiceImpl;
struct NodeModelRegistry;

struct RemoteReference {
  NodeModelImpl* reference_type = nullptr;
  NodeModelImpl* target = nullptr;
  bool forward = true;
};

using ReferenceMap =
    std::map<scada::NodeId /*reference_type_id*/,
             std::map<scada::NodeId /*target_id*/,
                      scada::NodeId /*child_reference_type_id*/>>;

// One node of a NodeServiceImpl: its cached NodeState plus the fetch state
// machine that loads it.
//
// The model is the service's residency unit — it is held by `shared_ptr` from
// the keep-alive window, in-flight fetch coroutines and pending fetches, and
// tracked weakly by `NodeModelRegistry`. `NodeRef` cursors do not hold it (see
// node_ref.h); the service answers per-node operations by looking the model up.
//
// Loading goes through the service's injected NodeFetcher. Callers request a
// fetch depth with Fetch()/StartFetch(); requests that the current
// `fetch_status_` already satisfies complete immediately, the rest are queued
// in `fetch_callbacks_` and drained by NotifyCallbacks() once the status
// advances. Pending model/semantic/state notifications are held while a fetch
// batch holds the callback lock and flushed in OnFetchCompleted().
class NodeModelImpl final : public std::enable_shared_from_this<NodeModelImpl> {
 public:
  NodeModelImpl(NodeServiceImpl& service,
                std::shared_ptr<NodeModelRegistry> registry,
                scada::NodeId node_id);
  ~NodeModelImpl();

  // Identifier of the node this model represents.
  const scada::NodeId& node_id() const SCADA_LIFETIME_BOUND { return node_id_; }

  void OnModelChanged(const scada::ModelChangeEvent& event);

  void OnFetched(const scada::NodeState& node_state);
  void OnFetchCompleted();
  void OnFetchError(scada::Status&& status);
  void OnChildrenFetched(scada::ReferenceDescriptions&& references);

  // --- Fetch state ---------------------------------------------------------

  scada::Status GetStatus() const { return status_; }
  NodeFetchStatus GetFetchStatus() const { return fetch_status_; }

  // Requests that the node be loaded at least to |requested_status|. Fetch()
  // resumes once the status covers the request; StartFetch() is the
  // fire-and-forget form.
  Awaitable<void> Fetch(const NodeFetchStatus& requested_status);
  void StartFetch(const NodeFetchStatus& requested_status);

  // Marks the node gone: every outstanding request completes with
  // Bad_WrongNodeId rather than hanging.
  void OnNodeDeleted();

  // --- Per-node reads (invoked by the service on behalf of a NodeRef). ------

  scada::Variant GetAttribute(scada::AttributeId attribute_id) const;
  NodeRef GetDataType() const;
  NodeRef GetAggregate(const scada::NodeId& aggregate_declaration_id) const;
  NodeRef GetChild(const scada::QualifiedName& child_name) const;
  NodeRef GetTarget(const scada::NodeId& reference_type_id,
                    bool forward) const;
  std::vector<NodeRef> GetTargets(const scada::NodeId& reference_type_id,
                                  bool forward) const;
  NodeRef::Reference GetReference(const scada::NodeId& reference_type_id,
                                  bool forward,
                                  const scada::NodeId& node_id) const;
  std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const;

 private:
  using FetchCallback = std::function<void()>;

  // Shared body of the two public fetch entry points. |callback| may be null.
  void StartFetch(const NodeFetchStatus& requested_status,
                  FetchCallback callback);

  // Asks the service to spawn the fetch coroutines for this node.
  void OnFetchRequested(const NodeFetchStatus& requested_status);

  // Records a new fetch outcome and drains any request it now satisfies.
  void SetFetchStatus(const scada::Status& status,
                      const NodeFetchStatus& fetch_status);

  // Completes every queued request that |fetch_status_| now covers.
  void NotifyCallbacks();

  NodeRef GetAggregateDeclaration(
      const scada::NodeId& aggregate_declaration_id) const;

  // True if the edge is already recorded, either in the fetched node state or
  // among the fetched children. Used to keep each edge single: node fetches
  // of neighbors push mirror references into resident models, which must not
  // duplicate what this model's own fetches reported.
  bool HasReference(const scada::ReferenceDescription& reference) const;

  void SetError(const scada::Status& status);

  void NotifyModelChanged();
  void NotifySemanticChanged();

  // Publishes a fresh immutable snapshot built from the working state and
  // notifies observers with it; deferred while a fetch batch holds the
  // callback lock.
  void NotifyStateChanged();

  NodeServiceImpl& service_;
  // Shared with the service so the destructor can unregister safely even if
  // the last NodeRef outlives the service.
  const std::shared_ptr<NodeModelRegistry> registry_;
  const scada::NodeId node_id_;

  // Outcome of the last fetch, and how much of the node it loaded.
  scada::Status status_{scada::StatusCode::Good};
  NodeFetchStatus fetch_status_{};

  // Depth already requested from the service; unioned across callers so a
  // second, deeper request while a fetch is in flight is not lost.
  NodeFetchStatus fetching_status_{};

  // Requests waiting for `fetch_status_` to cover them.
  std::vector<std::pair<NodeFetchStatus, FetchCallback>> fetch_callbacks_;

  // Non-zero while a fetch batch is applying results: notifications are held
  // as `pending_*` below and flushed in OnFetchCompleted(), so consumers never
  // observe a half-applied state.
  int callback_lock_count_ = 0;

  // Guards NotifyCallbacks() against synchronous re-entry. A callback that
  // calls Fetch() on this model (directly or via a child) would otherwise run
  // NotifyCallbacks nested in its own stack frame; the outermost frame owns
  // the drain and picks up whatever the nested callback enqueued, so the stack
  // cannot grow without bound.
  bool notifying_callbacks_ = false;

  // Working state, mutated in place by fetch results and remote updates.
  // Immutable snapshots of it are published to observers via
  // NotifyStateChanged.
  scada::NodeState node_state_;
  scada::ReferenceDescriptions child_references_;

  std::shared_ptr<bool> reference_request_;

  bool pending_model_changed_ = false;
  bool pending_semantic_changed_ = false;
  bool pending_state_changed_ = false;

  friend class NodeServiceImpl;
};

}  // namespace v3
