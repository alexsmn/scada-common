#include "node_service/static/static_node_service.h"

#include "base/check.h"
#include "base/map_util.h"
#include "common/data_services_util.h"
#include "model/nested_node_ids.h"
#include "node_service/static/static_node_model.h"

namespace {

void RelocateReferences(scada::NodeState& node_state) {
  if (!node_state.type_definition_id.is_null()) {
    node_state.references.emplace_back(
        /*reference_type_id=*/scada::id::HasTypeDefinition,
        /*forward=*/true,
        /*node_id=*/std::move(node_state.type_definition_id));

    node_state.type_definition_id = {};
  }

  if (!node_state.parent_id.is_null()) {
    base::Check(!node_state.reference_type_id.is_null());

    node_state.references.emplace_back(
        /*reference_type_id=*/std::move(node_state.reference_type_id),
        /*forward=*/false,
        /*node_id=*/std::move(node_state.parent_id));

    node_state.parent_id = {};
    node_state.reference_type_id = {};
  }
}

}  // namespace

StaticNodeService::StaticNodeService() = default;

StaticNodeService::StaticNodeService(scada::services services)
    : StaticNodeService{data_services::FromUnownedServices(services)} {}

StaticNodeService::StaticNodeService(DataServices data_services)
    : data_services_{std::move(data_services)} {}

void StaticNodeService::Add(scada::NodeState node_state) {
  base::Check(!node_state.node_id.is_null());
  base::Check(scada::IsInstance(node_state.node_class) ^
              node_state.type_definition_id.is_null());

  RelocateReferences(node_state);

  for (const auto& desc : node_state.references) {
    AddInverseReference(node_state.node_id, desc);
  }

  if (scada::IsInstance(node_state.node_class)) {
    for (const auto& [prop_decl_id, prop_value] : node_state.properties) {
      Add(MakePropNodeState(node_state.node_id, prop_decl_id, prop_value));
    }
  }

  node_state.properties.clear();

  auto node_id = node_state.node_id;

  nodes_.try_emplace(std::move(node_id), std::make_shared<StaticNodeModel>(
                                             *this, std::move(node_state)));
}

void StaticNodeService::AddAll(std::span<const scada::NodeState> node_states) {
  for (const auto& node_state : node_states) {
    Add(node_state);
  }
}

NodeRef StaticNodeService::GetNode(const scada::NodeId& node_id) {
  auto i = nodes_.find(node_id);
  return i != nodes_.end() ? NodeRef{node_id, this} : nullptr;
}

std::shared_ptr<const StaticNodeModel> StaticNodeService::FindNode(
    const scada::NodeId& node_id) const {
  auto i = nodes_.find(node_id);
  return i != nodes_.end() ? i->second : nullptr;
}

scada::Status StaticNodeService::GetStatus(const scada::NodeId& node_id) {
  auto node = FindNode(node_id);
  return node ? node->GetStatus()
              : scada::Status{scada::StatusCode::Bad_WrongNodeId};
}

NodeFetchStatus StaticNodeService::GetFetchStatus(
    const scada::NodeId& node_id) {
  auto node = FindNode(node_id);
  return node ? node->GetFetchStatus() : NodeFetchStatus{};
}

Awaitable<void> StaticNodeService::Fetch(
    const scada::NodeId& node_id,
    const NodeFetchStatus& requested_status) {
  // All static nodes are fully materialized on Add(); nothing to fetch.
  co_return;
}

void StaticNodeService::StartFetch(const scada::NodeId& node_id,
                                   const NodeFetchStatus& requested_status) {}

scada::Variant StaticNodeService::GetAttribute(
    const scada::NodeId& node_id,
    scada::AttributeId attribute_id) {
  auto node = FindNode(node_id);
  return node ? node->GetAttribute(attribute_id) : scada::Variant{};
}

NodeRef StaticNodeService::GetDataType(const scada::NodeId& node_id) {
  auto node = FindNode(node_id);
  return node ? node->GetDataType() : nullptr;
}

NodeRef::Reference StaticNodeService::GetReference(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward,
    const scada::NodeId& target_id) {
  auto node = FindNode(node_id);
  return node ? node->GetReference(reference_type_id, forward, target_id)
              : NodeRef::Reference{};
}

std::vector<NodeRef::Reference> StaticNodeService::GetReferences(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward) {
  auto node = FindNode(node_id);
  return node ? node->GetReferences(reference_type_id, forward)
              : std::vector<NodeRef::Reference>{};
}

NodeRef StaticNodeService::GetTarget(const scada::NodeId& node_id,
                                     const scada::NodeId& reference_type_id,
                                     bool forward) {
  auto node = FindNode(node_id);
  return node ? node->GetTarget(reference_type_id, forward) : nullptr;
}

std::vector<NodeRef> StaticNodeService::GetTargets(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward) {
  auto node = FindNode(node_id);
  return node ? node->GetTargets(reference_type_id, forward)
              : std::vector<NodeRef>{};
}

NodeRef StaticNodeService::GetAggregate(
    const scada::NodeId& node_id,
    const scada::NodeId& aggregate_declaration_id) {
  return GetNode(MakeAggregateId(node_id, aggregate_declaration_id));
}

NodeRef StaticNodeService::GetChild(const scada::NodeId& node_id,
                                    const scada::QualifiedName& child_name) {
  auto node = FindNode(node_id);
  return node ? node->GetChild(child_name) : NodeRef{};
}

scada::node StaticNodeService::GetScadaNode(const scada::NodeId& node_id) {
  auto node = FindNode(node_id);
  return node ? node->GetScadaNode() : scada::node{};
}

scada::NodeId StaticNodeService::MakeAggregateId(
    const scada::NodeId& node_id,
    const scada::NodeId& prop_decl_id) const {
  return MakeNestedNodeId(node_id, prop_decl_id.ToString());
}

// static
scada::NodeState StaticNodeService::MakePropNodeState(
    const scada::NodeId& node_id,
    const scada::NodeId& prop_decl_id,
    const scada::Variant& prop_value) {
  return {.node_id = MakeAggregateId(node_id, prop_decl_id),
          .node_class = scada::NodeClass::Variable,
          .type_definition_id = scada::id::PropertyType,
          .parent_id = node_id,
          .reference_type_id = scada::id::HasProperty,
          .attributes = {.value = prop_value}};
}

NodeRef::Reference StaticNodeService::GetReference(
    const scada::ReferenceDescription& desc) {
  return {.reference_type = GetNode(desc.reference_type_id),
          .target = GetNode(desc.node_id),
          .forward = desc.forward};
}

const std::set<scada::ReferenceDescription>&
StaticNodeService::inverse_references(const scada::NodeId& node_id) const {
  static const std::set<scada::ReferenceDescription> empty_set;
  auto i = inverse_references_.find(node_id);
  return i != inverse_references_.end() ? i->second : empty_set;
}

void StaticNodeService::AddInverseReference(
    const scada::NodeId& node_id,
    const scada::ReferenceDescription& desc) {
  base::Check(!node_id.is_null());
  base::Check(!desc.reference_type_id.is_null());
  base::Check(!desc.node_id.is_null());

  inverse_references_[desc.node_id].emplace(
      /*reference_type_id=*/desc.reference_type_id,
      /*forward=*/!desc.forward,
      /*node_id=*/node_id);
}
