#include "node_service/node_subscription_table.h"

#include "scada/event.h"

std::shared_ptr<NodeSubscriptionTable::Entry> NodeSubscriptionTable::Ensure(
    const scada::NodeId& node_id) {
  auto& weak = entries_[node_id];
  if (auto entry = weak.lock())
    return entry;

  // First subscription for this node: create the bundle. The Entry back-refs
  // the table and its id so it can erase its own map slot on destruction.
  auto entry = std::shared_ptr<Entry>(new Entry{this, node_id, {}});
  weak = entry;
  return entry;
}

std::shared_ptr<NodeSubscriptionTable::Entry> NodeSubscriptionTable::Find(
    const scada::NodeId& node_id) const {
  auto i = entries_.find(node_id);
  return i != entries_.end() ? i->second.lock() : nullptr;
}

boost::signals2::scoped_connection NodeSubscriptionTable::SubscribeModelChanged(
    const scada::NodeId& node_id,
    const ModelChangedCallback& callback) {
  auto entry = Ensure(node_id);
  // Capture the entry into the slot so it stays resident exactly while this
  // (or any sibling) subscription is live; the last release erases the entry.
  return entry->signals_.model_changed.connect(
      [entry, callback](const scada::ModelChangeEvent& event) {
        callback(event);
      });
}

boost::signals2::scoped_connection
NodeSubscriptionTable::SubscribeNodeSemanticChanged(
    const scada::NodeId& node_id,
    const NodeSemanticChangedCallback& callback) {
  auto entry = Ensure(node_id);
  return entry->signals_.node_semantic_changed.connect(
      [entry, callback](const scada::NodeId& changed_id) {
        callback(changed_id);
      });
}

boost::signals2::scoped_connection NodeSubscriptionTable::SubscribeNodeFetched(
    const scada::NodeId& node_id,
    const NodeFetchedCallback& callback) {
  auto entry = Ensure(node_id);
  return entry->signals_.node_fetched.connect(
      [entry, callback](const NodeFetchedEvent& event) { callback(event); });
}

boost::signals2::scoped_connection
NodeSubscriptionTable::SubscribeNodeStateChanged(
    const scada::NodeId& node_id,
    const NodeStateChangedCallback& callback) {
  auto entry = Ensure(node_id);
  return entry->signals_.node_state_changed.connect(
      [entry, callback](const NodeStateChangedEvent& event) {
        callback(event);
      });
}

void NodeSubscriptionTable::EmitModelChanged(
    const scada::ModelChangeEvent& event) {
  // Locking the entry keeps it alive for the duration of the emit, so a
  // callback that releases its own connection cannot destroy the signal
  // mid-emission.
  if (auto entry = Find(event.node_id))
    entry->signals_.model_changed(event);
}

void NodeSubscriptionTable::EmitNodeSemanticChanged(
    const scada::NodeId& node_id) {
  if (auto entry = Find(node_id))
    entry->signals_.node_semantic_changed(node_id);
}

void NodeSubscriptionTable::EmitNodeFetched(const NodeFetchedEvent& event) {
  if (auto entry = Find(event.node_id))
    entry->signals_.node_fetched(event);
}

void NodeSubscriptionTable::EmitNodeStateChanged(
    const NodeStateChangedEvent& event) {
  if (auto entry = Find(event.node_id))
    entry->signals_.node_state_changed(event);
}
