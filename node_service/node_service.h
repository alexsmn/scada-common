#pragma once

#include "base/awaitable.h"
#include "node_service/node_events.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_ref.h"
#include "scada/attribute_ids.h"
#include "scada/node.h"
#include "scada/node_id.h"

#include <boost/signals2/connection.hpp>
#include <vector>

// Service that owns the node graph and answers per-node operations.
//
// `NodeRef` is a lightweight `{NodeId, NodeService*}` cursor that forwards
// every operation to its service through the per-node methods below. The
// service owns the cached per-node state internally; holding a `NodeRef` does
// not by itself keep a node resident (see the node-scoped Subscribe* methods,
// which pin a node for the lifetime of the subscription).
//
// Implementations are executor-affine: a service and the NodeRefs it hands out
// must be used from the executor the service was created on.
class NodeService {
 public:
  virtual ~NodeService() = default;

  // Returns a cursor to |node_id|. Ensures the node is resident and kicks off
  // its node fetch, matching the pre-cursor GetNode side effects.
  virtual NodeRef GetNode(const scada::NodeId& node_id) = 0;

  // --- Per-node operations (invoked by NodeRef with its stored node id). ---
  // These may create/touch the internal cache entry, so they are non-const.

  virtual scada::Status GetStatus(const scada::NodeId& node_id) = 0;
  virtual NodeFetchStatus GetFetchStatus(const scada::NodeId& node_id) = 0;

  virtual Awaitable<void> Fetch(const scada::NodeId& node_id,
                                const NodeFetchStatus& requested_status) = 0;
  virtual void StartFetch(const scada::NodeId& node_id,
                          const NodeFetchStatus& requested_status) = 0;

  virtual scada::Variant GetAttribute(const scada::NodeId& node_id,
                                      scada::AttributeId attribute_id) = 0;

  virtual NodeRef GetDataType(const scada::NodeId& node_id) = 0;

  virtual NodeRef::Reference GetReference(const scada::NodeId& node_id,
                                          const scada::NodeId& reference_type_id,
                                          bool forward,
                                          const scada::NodeId& target_id) = 0;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& node_id,
      const scada::NodeId& reference_type_id,
      bool forward) = 0;

  virtual NodeRef GetTarget(const scada::NodeId& node_id,
                            const scada::NodeId& reference_type_id,
                            bool forward) = 0;
  virtual std::vector<NodeRef> GetTargets(const scada::NodeId& node_id,
                                          const scada::NodeId& reference_type_id,
                                          bool forward) = 0;

  virtual NodeRef GetAggregate(const scada::NodeId& node_id,
                               const scada::NodeId& aggregate_declaration_id) = 0;
  virtual NodeRef GetChild(const scada::NodeId& node_id,
                           const scada::QualifiedName& child_name) = 0;

  virtual scada::node GetScadaNode(const scada::NodeId& node_id) = 0;

  // --- Node-scoped subscriptions. ---
  // An active subscription keeps the node resident for the lifetime of the
  // returned connection.

  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeModelChanged(
      const scada::NodeId& node_id, const ModelChangedCallback& callback) = 0;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const scada::NodeId& node_id,
      const NodeSemanticChangedCallback& callback) = 0;
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const scada::NodeId& node_id, const NodeFetchedCallback& callback) = 0;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(const scada::NodeId& node_id,
                            const NodeStateChangedCallback& callback) = 0;

  // --- Service-wide subscriptions (all nodes). ---

  // Notifies about node graph changes (nodes/references added or deleted).
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeModelChanged(const ModelChangedCallback& callback) const = 0;
  // Notifies about displayed attribute changes and fetch errors.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const NodeSemanticChangedCallback& callback) const = 0;
  // Notifies when a node fetch completed (successfully or with an error).
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const NodeFetchedCallback& callback) const = 0;
  // Notifies after a node swapped in a new state snapshot.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(const NodeStateChangedCallback& callback) const = 0;

  virtual size_t GetPendingTaskCount() const = 0;
};
