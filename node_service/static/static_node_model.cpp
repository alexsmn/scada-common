#include "node_service/static/static_node_model.h"

#include "model/nested_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/static/static_node_service.h"

#include <ranges>

namespace {

auto ToVector(auto&& range) {
  return std::vector(range.begin(), range.end());
}

auto FilterReferences(const scada::NodeId& ref_type_id, bool forward) {
  return std::views::filter(
      [ref_type_id, forward](const scada::ReferenceDescription& desc) {
        return desc.forward == forward && desc.reference_type_id == ref_type_id;
      });
}

}  // namespace

StaticNodeModel::StaticNodeModel(StaticNodeService& service,
                                 scada::NodeState node_state)
    : service_{service}, node_state_{std::move(node_state)} {
  fetch_status_ = NodeFetchStatus::Max();
}

scada::Variant StaticNodeModel::GetAttribute(
    scada::AttributeId attribute_id) const {
  return node_state_.GetAttribute(attribute_id).value_or(scada::Variant{});
}

NodeRef StaticNodeModel::GetDataType() const {
  return service_.GetNode(node_state_.attributes.data_type);
}

NodeRef::Reference StaticNodeModel::GetReference(
    const scada::NodeId& reference_type_id,
    bool forward,
    const scada::NodeId& node_id) const {
  auto refs = GetReferences(reference_type_id, forward);
  auto i = std::ranges::find(refs, node_id, [&](const NodeRef::Reference& ref) {
    return ref.forward == forward && ref.reference_type.node_id() == node_id &&
           ref.target.node_id() == node_id;
  });
  return i != refs.end() ? *i : NodeRef::Reference{};
}

std::vector<NodeRef::Reference> StaticNodeModel::GetReferences(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  auto refs =
      node_state_.references | FilterReferences(reference_type_id, forward);

  auto inverse_refs = service_.inverse_references(node_state_.node_id) |
                      FilterReferences(reference_type_id, forward);

  std::vector<scada::ReferenceDescription> all_refs;
  all_refs.insert(all_refs.end(), refs.begin(), refs.end());
  all_refs.insert(all_refs.end(), inverse_refs.begin(), inverse_refs.end());

  auto all_service_refs =
      all_refs | std::views::transform(std::bind_front(
                     &StaticNodeService::GetReference, &service_));

  return std::vector<NodeRef::Reference>(all_service_refs.begin(),
                                         all_service_refs.end());
}

NodeRef StaticNodeModel::GetTarget(const scada::NodeId& reference_type_id,
                                   bool forward) const {
  auto refs = GetReferences(reference_type_id, forward);
  return refs.empty() ? NodeRef{} : refs.front().target;
}

std::vector<NodeRef> StaticNodeModel::GetTargets(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  auto targets = GetReferences(reference_type_id, forward) |
                 std::views::transform(&NodeRef::Reference::target);

  return std::vector<NodeRef>(targets.begin(), targets.end());
}

NodeRef StaticNodeModel::GetAggregate(
    const scada::NodeId& aggregate_declaration_id) const {
  return service_.GetAggregate(node_state_.node_id, aggregate_declaration_id);
}

NodeRef StaticNodeModel::GetChild(
    const scada::QualifiedName& child_name) const {
  // Not implemented.
  assert(false);
  return {};
}

scada::node StaticNodeModel::GetScadaNode() const {
  if (const auto& node_id =
          GetAttribute(scada::AttributeId::NodeId).as_node_id();
      !node_id.is_null()) {
    return scada::client{service_.services_}.node(node_id);
  } else {
    return {};
  }
}
