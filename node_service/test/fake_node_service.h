#pragma once

#include "base/awaitable.h"
#include "common/node_state.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/standard_node_ids.h"

#include <map>
#include <vector>

// Data-backed NodeService for tests.
//
// Register nodes with Add(NodeState); the service answers attribute reads and
// graph navigation from the stored states, resolving reference types by subtype
// (via node_util::IsSubtypeOf), just like the production services. NodeRefs
// handed out are plain {NodeId, this} cursors, so identity, hashing, and
// subscriptions route through this fake exactly as through a real service.
//
// Node fetches complete immediately (every node reports fully fetched). Tests
// can drive change notifications with the Emit* helpers.
class FakeNodeService : public NodeService {
 public:
  // Registers or replaces a node's state. Returns a cursor to it. References
  // authored on |state| are mirrored onto their targets so inverse navigation
  // works regardless of registration order.
  NodeRef Add(scada::NodeState state) {
    scada::NodeId node_id = state.node_id;

    if (!state.type_definition_id.is_null()) {
      state.references.push_back(scada::ReferenceDescription{
          scada::id::HasTypeDefinition, true, state.type_definition_id});
    }
    if (!state.parent_id.is_null()) {
      state.references.push_back(scada::ReferenceDescription{
          state.reference_type_id, false, state.parent_id});
    }
    if (!state.supertype_id.is_null()) {
      state.references.push_back(scada::ReferenceDescription{
          scada::id::HasSubtype, false, state.supertype_id});
    }

    // Mirror every reference onto the far endpoint's inverse index.
    for (const auto& ref : state.references) {
      inverse_references_[ref.node_id].push_back(scada::ReferenceDescription{
          ref.reference_type_id, !ref.forward, node_id});
    }

    nodes_[node_id].state = std::move(state);
    return NodeRef{node_id, this};
  }

  // Emits change notifications for a registered node (per-node + service-wide).
  void EmitNodeStateChanged(const scada::NodeId& node_id) {
    auto* entry = Find(node_id);
    if (!entry)
      return;
    NodeStateChangedEvent event{
        node_id, std::make_shared<const scada::NodeState>(entry->state),
        NodeFetchStatus::Max};
    entry->signals.node_state_changed(event);
    service_signals_.node_state_changed(event);
  }
  void EmitNodeSemanticChanged(const scada::NodeId& node_id) {
    if (auto* entry = Find(node_id))
      entry->signals.node_semantic_changed(node_id);
    service_signals_.node_semantic_changed(node_id);
  }
  void EmitNodeFetched(const scada::NodeId& node_id) {
    if (auto* entry = Find(node_id))
      entry->signals.node_fetched({node_id});
    service_signals_.node_fetched({node_id});
  }
  void EmitModelChanged(const scada::ModelChangeEvent& event) {
    if (auto* entry = Find(event.node_id))
      entry->signals.model_changed(event);
    service_signals_.model_changed(event);
  }

  // NodeService
  NodeRef GetNode(const scada::NodeId& node_id) override {
    return node_id.is_null() ? nullptr : NodeRef{node_id, this};
  }

  scada::Status GetStatus(const scada::NodeId& node_id) override {
    return scada::StatusCode::Good;
  }
  NodeFetchStatus GetFetchStatus(const scada::NodeId& node_id) override {
    return NodeFetchStatus::Max;
  }
  Awaitable<void> Fetch(const scada::NodeId& node_id,
                        const NodeFetchStatus& requested_status) override {
    co_return;
  }
  void StartFetch(const scada::NodeId& node_id,
                  const NodeFetchStatus& requested_status) override {}

  scada::Variant GetAttribute(const scada::NodeId& node_id,
                              scada::AttributeId attribute_id) override {
    if (attribute_id == scada::AttributeId::NodeId)
      return node_id;
    auto* entry = Find(node_id);
    return entry ? entry->state.GetAttribute(attribute_id).value_or(
                       scada::Variant{})
                 : scada::Variant{};
  }

  NodeRef GetDataType(const scada::NodeId& node_id) override {
    auto* entry = Find(node_id);
    return entry ? GetNode(entry->state.attributes.data_type) : nullptr;
  }

  NodeRef::Reference GetReference(const scada::NodeId& node_id,
                                  const scada::NodeId& reference_type_id,
                                  bool forward,
                                  const scada::NodeId& target_id) override {
    auto refs = GetReferences(node_id, reference_type_id, forward);
    if (refs.empty())
      return {};
    if (target_id.is_null())
      return refs.front();
    for (auto& ref : refs) {
      if (ref.target.node_id() == target_id)
        return ref;
    }
    return {};
  }

  std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& node_id,
      const scada::NodeId& reference_type_id,
      bool forward) override {
    std::vector<NodeRef::Reference> result;
    auto add_matching = [&](const scada::ReferenceDescription& ref) {
      if (ref.forward != forward)
        return;
      NodeRef reference_type = GetNode(ref.reference_type_id);
      if (IsSubtypeOf(reference_type, reference_type_id)) {
        result.push_back(
            {reference_type, GetNode(ref.node_id), ref.forward});
      }
    };
    if (auto* entry = Find(node_id)) {
      for (const auto& ref : entry->state.references)
        add_matching(ref);
    }
    if (auto i = inverse_references_.find(node_id);
        i != inverse_references_.end()) {
      for (const auto& ref : i->second)
        add_matching(ref);
    }
    return result;
  }

  NodeRef GetTarget(const scada::NodeId& node_id,
                    const scada::NodeId& reference_type_id,
                    bool forward) override {
    auto* entry = Find(node_id);
    if (entry) {
      if (forward && reference_type_id == scada::id::HasTypeDefinition)
        return GetNode(entry->state.type_definition_id);
      if (!forward && reference_type_id == scada::id::HasSubtype)
        return GetNode(entry->state.supertype_id);
      if (!forward && reference_type_id == scada::id::HierarchicalReferences) {
        if (!entry->state.parent_id.is_null())
          return GetNode(entry->state.parent_id);
        if (!entry->state.supertype_id.is_null())
          return GetNode(entry->state.supertype_id);
      }
    }
    return GetReference(node_id, reference_type_id, forward, {}).target;
  }

  std::vector<NodeRef> GetTargets(const scada::NodeId& node_id,
                                  const scada::NodeId& reference_type_id,
                                  bool forward) override {
    std::vector<NodeRef> result;
    for (const auto& ref : GetReferences(node_id, reference_type_id, forward))
      result.push_back(ref.target);
    return result;
  }

  NodeRef GetAggregate(const scada::NodeId& node_id,
                       const scada::NodeId& aggregate_declaration_id) override {
    return nullptr;
  }
  NodeRef GetChild(const scada::NodeId& node_id,
                   const scada::QualifiedName& child_name) override {
    for (const auto& child :
         GetTargets(node_id, scada::id::HierarchicalReferences, true)) {
      if (child.browse_name() == child_name)
        return child;
    }
    return {};
  }
  scada::node GetScadaNode(const scada::NodeId& node_id) override { return {}; }

  boost::signals2::scoped_connection SubscribeModelChanged(
      const scada::NodeId& node_id,
      const ModelChangedCallback& callback) override {
    return nodes_[node_id].signals.model_changed.connect(callback);
  }
  boost::signals2::scoped_connection SubscribeNodeSemanticChanged(
      const scada::NodeId& node_id,
      const NodeSemanticChangedCallback& callback) override {
    return nodes_[node_id].signals.node_semantic_changed.connect(callback);
  }
  boost::signals2::scoped_connection SubscribeNodeFetched(
      const scada::NodeId& node_id,
      const NodeFetchedCallback& callback) override {
    return nodes_[node_id].signals.node_fetched.connect(callback);
  }
  boost::signals2::scoped_connection SubscribeNodeStateChanged(
      const scada::NodeId& node_id,
      const NodeStateChangedCallback& callback) override {
    return nodes_[node_id].signals.node_state_changed.connect(callback);
  }

  boost::signals2::scoped_connection SubscribeModelChanged(
      const ModelChangedCallback& callback) const override {
    return service_signals_.model_changed.connect(callback);
  }
  boost::signals2::scoped_connection SubscribeNodeSemanticChanged(
      const NodeSemanticChangedCallback& callback) const override {
    return service_signals_.node_semantic_changed.connect(callback);
  }
  boost::signals2::scoped_connection SubscribeNodeFetched(
      const NodeFetchedCallback& callback) const override {
    return service_signals_.node_fetched.connect(callback);
  }
  boost::signals2::scoped_connection SubscribeNodeStateChanged(
      const NodeStateChangedCallback& callback) const override {
    return service_signals_.node_state_changed.connect(callback);
  }

  size_t GetPendingTaskCount() const override { return 0; }

 private:
  struct Entry {
    scada::NodeState state;
    NodeSignals signals;
  };

  Entry* Find(const scada::NodeId& node_id) {
    auto i = nodes_.find(node_id);
    return i != nodes_.end() ? &i->second : nullptr;
  }

  std::map<scada::NodeId, Entry> nodes_;
  std::map<scada::NodeId, std::vector<scada::ReferenceDescription>>
      inverse_references_;
  mutable NodeSignals service_signals_;
};
