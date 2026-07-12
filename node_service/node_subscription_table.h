#pragma once

#include "node_service/node_events.h"
#include "scada/node_id.h"

#include <boost/signals2/connection.hpp>
#include <memory>
#include <unordered_map>

// Per-service registry of node event signals.
//
// A `NodeService` used to embed a four-signal `NodeSignals` bundle in every
// resident node model, paying that fixed storage whether or not anyone
// subscribed. With thousands of resident nodes that dominated memory. This
// table instead materializes an entry only while a node has at least one live
// subscription: subscribing to an unsubscribed node creates the entry,
// releasing the last subscription destroys it, and emitting for an
// unsubscribed node is a single map miss. It also decouples subscription
// lifetime from node-model residency — an entry stores signals only and does
// not pin a model.
//
// Executor-affine: every operation (subscribe, emit, and the connection
// release that erases an entry) must run on the owning service's executor, so
// the table needs no internal locking. The returned `scoped_connection` must
// likewise be released on that executor.
class NodeSubscriptionTable {
 public:
  NodeSubscriptionTable() = default;
  NodeSubscriptionTable(const NodeSubscriptionTable&) = delete;
  NodeSubscriptionTable& operator=(const NodeSubscriptionTable&) = delete;

  // Connects `callback` to `node_id`'s corresponding signal, materializing the
  // entry on first subscription. Releasing the returned connection drops the
  // entry once it was the node's last subscription.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeModelChanged(
      const scada::NodeId& node_id,
      const ModelChangedCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeSemanticChanged(
      const scada::NodeId& node_id,
      const NodeSemanticChangedCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeFetched(
      const scada::NodeId& node_id,
      const NodeFetchedCallback& callback);
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeStateChanged(
      const scada::NodeId& node_id,
      const NodeStateChangedCallback& callback);

  // Fires the corresponding per-node signal if the node has subscribers; a
  // single map lookup and no-op otherwise. The node id is taken from the event
  // where the event carries one.
  void EmitModelChanged(const scada::ModelChangeEvent& event);
  void EmitNodeSemanticChanged(const scada::NodeId& node_id);
  void EmitNodeFetched(const NodeFetchedEvent& event);
  void EmitNodeStateChanged(const NodeStateChangedEvent& event);

  // Number of nodes with at least one live subscription. Test/diagnostic aid.
  size_t entry_count() const { return entries_.size(); }

 private:
  // Holds one node's signal bundle. Self-erases from the owning table when its
  // last reference (a subscribing slot) drops, so `entries_` never accumulates
  // dead weak pointers beyond the erasing entry itself.
  struct Entry {
    NodeSubscriptionTable* owner;
    scada::NodeId node_id;
    // Note the trailing underscore: `signals` is a Qt keyword macro
    // (`#define signals public`), and this header is included by Qt client
    // code through v3::NodeServiceImpl.
    NodeSignals signals_;

    ~Entry() { owner->entries_.erase(node_id); }
  };

  // Returns the entry for `node_id`, creating it if absent. The caller must
  // capture the returned shared_ptr into the connecting slot so the entry
  // lives exactly as long as some subscription references it.
  std::shared_ptr<Entry> Ensure(const scada::NodeId& node_id);

  // Returns the entry if the node has subscribers, else nullptr.
  std::shared_ptr<Entry> Find(const scada::NodeId& node_id) const;

  // Weak: entries are kept alive by their subscribing slots, not by the map.
  std::unordered_map<scada::NodeId, std::weak_ptr<Entry>> entries_;
};
