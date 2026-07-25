#pragma once

#include "base/awaitable.h"
#include "common/node_state.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/client.h"
#include "scada/standard_node_ids.h"
#include "scada/standard_reference_types.h"

#include <functional>
#include <map>
#include <optional>
#include <vector>

// Data-backed NodeService for tests.
//
// Register nodes with Add(NodeState); the service answers attribute reads and
// graph navigation from the stored states, resolving reference types by subtype
// just like the production services. NodeRefs handed out are plain
// {NodeId, this} cursors, so identity, hashing, and subscriptions route through
// this fake exactly as through a real service.
//
// Nodes default to fully fetched (NodeFetchStatus::Max) and Good, so fetches
// complete immediately. Tests that need to exercise partially-fetched or failed
// nodes drive them with SetFetchStatus / SetStatus / SetFetchHandler, and can
// assert on what was requested via fetch_requests(). Change notifications are
// driven with the Emit* helpers.
class FakeNodeService : public NodeService {
 public:
  FakeNodeService() = default;

  // Backs GetScadaNode: a scada::node is a service-bound cursor, so it must
  // carry the services used for reads/writes/calls made through it. A
  // default-constructed fake yields nodes whose operations report
  // Bad_Disconnected.
  explicit FakeNodeService(scada::services services) : client_{services} {}

  // Registers or replaces a node's state. Returns a cursor to it. References
  // authored on |state| are mirrored onto their targets so inverse navigation
  // works regardless of registration order. Properties on an instance node are
  // materialized as child Variable nodes under a synthetic nested id, the way
  // StaticNodeService::Add does, so GetAggregate resolves them.
  NodeRef Add(scada::NodeState state) {
    scada::NodeId node_id = state.node_id;

    if (scada::IsInstance(state.node_class)) {
      for (const auto& [prop_decl_id, prop_value] : state.properties) {
        Add(scada::NodeState{
            .node_id = MakeAggregateId(node_id, prop_decl_id),
            .node_class = scada::NodeClass::Variable,
            .type_definition_id = scada::id::PropertyType,
            .parent_id = node_id,
            .reference_type_id = scada::id::HasProperty,
            .attributes = {.value = prop_value}});
      }
    }
    state.properties.clear();

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

  // --- Per-node fetch and status control ---------------------------------
  // Nodes default to fully fetched (NodeFetchStatus::Max) and Good; these
  // override that for one node so tests can drive partially-fetched, failed,
  // or slow-loading nodes.

  void SetFetchStatus(const scada::NodeId& node_id, NodeFetchStatus status) {
    nodes_[node_id].fetch_status = status;
  }
  void SetStatus(const scada::NodeId& node_id, scada::Status status) {
    nodes_[node_id].status = std::move(status);
  }

  // Replaces immediate completion of Fetch() for one node. The handler may
  // suspend, so a test can hold a fetch open and resume it later; it typically
  // calls SetFetchStatus() before resuming. StartFetch() records the request
  // but never runs the handler, matching the fire-and-forget production path.
  using FetchHandler = std::function<Awaitable<void>(const NodeFetchStatus&)>;
  void SetFetchHandler(const scada::NodeId& node_id, FetchHandler handler) {
    nodes_[node_id].fetch_handler = std::move(handler);
  }

  // Every status passed to Fetch() or StartFetch() for |node_id|, in order.
  // Replaces gmock call-shape expectations on the old per-node model.
  const std::vector<NodeFetchStatus>& fetch_requests(
      const scada::NodeId& node_id) const {
    static const std::vector<NodeFetchStatus> kNone;
    const Entry* entry = Find(node_id);
    return entry ? entry->fetch_requests : kNone;
  }
  void ClearFetchRequests() {
    for (auto& [node_id, entry] : nodes_)
      entry.fetch_requests.clear();
  }

  // Emits change notifications for a registered node (per-node + service-wide).
  void EmitNodeStateChanged(const scada::NodeId& node_id) {
    auto* entry = Find(node_id);
    if (!entry)
      return;
    NodeStateChangedEvent event{
        node_id, std::make_shared<const scada::NodeState>(entry->state),
        NodeFetchStatus::Max};
    entry->node_signals.node_state_changed(event);
    service_signals_.node_state_changed(event);
  }
  void EmitNodeSemanticChanged(const scada::NodeId& node_id) {
    if (auto* entry = Find(node_id))
      entry->node_signals.node_semantic_changed(node_id);
    service_signals_.node_semantic_changed(node_id);
  }
  void EmitNodeFetched(const scada::NodeId& node_id) {
    if (auto* entry = Find(node_id))
      entry->node_signals.node_fetched({node_id});
    service_signals_.node_fetched({node_id});
  }
  void EmitModelChanged(const scada::ModelChangeEvent& event) {
    if (auto* entry = Find(event.node_id))
      entry->node_signals.model_changed(event);
    service_signals_.model_changed(event);
  }

  // NodeService
  NodeRef GetNode(const scada::NodeId& node_id) override {
    return node_id.is_null() ? nullptr : NodeRef{node_id, this};
  }

  scada::Status GetStatus(const scada::NodeId& node_id) override {
    auto* entry = Find(node_id);
    return entry ? entry->status : scada::Status{scada::StatusCode::Good};
  }
  NodeFetchStatus GetFetchStatus(const scada::NodeId& node_id) override {
    auto* entry = Find(node_id);
    return entry ? entry->fetch_status : NodeFetchStatus::Max;
  }
  Awaitable<void> Fetch(const scada::NodeId& node_id,
                        const NodeFetchStatus& requested_status) override {
    // Copy the handler out before suspending: it may re-enter the service
    // (Add, SetFetchHandler) and rehash `nodes_`, dangling the entry.
    FetchHandler handler;
    if (auto* entry = Find(node_id)) {
      entry->fetch_requests.push_back(requested_status);
      handler = entry->fetch_handler;
    }
    if (handler)
      co_await handler(requested_status);
  }
  void StartFetch(const scada::NodeId& node_id,
                  const NodeFetchStatus& requested_status) override {
    if (auto* entry = Find(node_id))
      entry->fetch_requests.push_back(requested_status);
  }

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
      if (MatchesReferenceType(ref.reference_type_id, reference_type_id)) {
        result.push_back(
            {GetNode(ref.reference_type_id), GetNode(ref.node_id), ref.forward});
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
    // Properties materialized from NodeState::properties live under a
    // synthetic nested id keyed by the declaration id. Test for existence with
    // Find(), not GetNode(): unlike the production services, GetNode() here
    // hands out a cursor for any non-null id.
    scada::NodeId property_id = MakeAggregateId(node_id, aggregate_declaration_id);
    if (Find(property_id))
      return NodeRef{std::move(property_id), this};

    // Properties created by a node factory instead live under their
    // declaration's browse name; resolve through it, as the real services do.
    if (Find(aggregate_declaration_id))
      return GetChild(node_id, GetNode(aggregate_declaration_id).browse_name());

    return {};
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
  scada::node GetScadaNode(const scada::NodeId& node_id) override {
    return node_id.is_null() ? scada::node{} : client_.node(node_id);
  }

  boost::signals2::scoped_connection SubscribeModelChanged(
      const scada::NodeId& node_id,
      const ModelChangedCallback& callback) override {
    return nodes_[node_id].node_signals.model_changed.connect(callback);
  }
  boost::signals2::scoped_connection SubscribeNodeSemanticChanged(
      const scada::NodeId& node_id,
      const NodeSemanticChangedCallback& callback) override {
    return nodes_[node_id].node_signals.node_semantic_changed.connect(callback);
  }
  boost::signals2::scoped_connection SubscribeNodeFetched(
      const scada::NodeId& node_id,
      const NodeFetchedCallback& callback) override {
    return nodes_[node_id].node_signals.node_fetched.connect(callback);
  }
  boost::signals2::scoped_connection SubscribeNodeStateChanged(
      const scada::NodeId& node_id,
      const NodeStateChangedCallback& callback) override {
    return nodes_[node_id].node_signals.node_state_changed.connect(callback);
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
    // Not `signals`: Qt's <QObject> defines that as a macro (`#define signals
    // public`), which makes this header uncompilable from any Qt translation
    // unit that includes it after a Qt header.
    NodeSignals node_signals;
    NodeFetchStatus fetch_status = NodeFetchStatus::Max;
    scada::Status status{scada::StatusCode::Good};
    FetchHandler fetch_handler;
    std::vector<NodeFetchStatus> fetch_requests;
  };

  static scada::NodeId MakeAggregateId(const scada::NodeId& node_id,
                                       const scada::NodeId& prop_decl_id) {
    return MakeNestedNodeId(node_id, prop_decl_id.ToString());
  }

  // Mirrors the production services (see StaticNodeModel): standard (ns0)
  // reference types resolve statically, so a HierarchicalReferences query
  // matches an Organizes / HasComponent edge without those ReferenceType nodes
  // being registered. Custom types fall back to a HasSubtype walk over the
  // registered graph.
  bool MatchesReferenceType(const scada::NodeId& ref_type_id,
                            const scada::NodeId& queried_id) {
    if (std::optional<bool> known =
            scada::IsStandardReferenceSubtype(ref_type_id, queried_id)) {
      return *known;
    }
    return IsSubtypeOf(GetNode(ref_type_id), queried_id);
  }

  Entry* Find(const scada::NodeId& node_id) {
    auto i = nodes_.find(node_id);
    return i != nodes_.end() ? &i->second : nullptr;
  }
  const Entry* Find(const scada::NodeId& node_id) const {
    auto i = nodes_.find(node_id);
    return i != nodes_.end() ? &i->second : nullptr;
  }

  std::map<scada::NodeId, Entry> nodes_;
  std::map<scada::NodeId, std::vector<scada::ReferenceDescription>>
      inverse_references_;
  mutable NodeSignals service_signals_;
  scada::client client_;
};
