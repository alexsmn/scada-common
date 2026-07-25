#include "node_service/static/static_node_model.h"

#include "node_service/static/static_node_service.h"
#include "scada/client.h"
#include "scada/standard_reference_types.h"

#include <optional>
#include <ranges>

namespace {

auto ToVector(auto&& range) {
  return std::vector(range.begin(), range.end());
}

auto FilterReferences(const scada::NodeId& ref_type_id, bool forward) {
  return std::views::filter(
      [ref_type_id, forward](const scada::ReferenceDescription& desc) {
        if (desc.forward != forward)
          return false;
        // Match reference-type subtypes, not just the exact id — a query for a
        // base type (e.g. HierarchicalReferences) must return Organizes /
        // HasComponent references. Standard (ns0) types resolve statically;
        // custom types fall back to an exact-id match.
        if (std::optional<bool> known = scada::IsStandardReferenceSubtype(
                desc.reference_type_id, ref_type_id)) {
          return *known;
        }
        return desc.reference_type_id == ref_type_id;
      });
}

}  // namespace

StaticNodeModel::StaticNodeModel(StaticNodeService& service,
                                 scada::NodeState node_state)
    : service_{service}, node_state_{std::move(node_state)} {}

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
  // GetReferences already filtered by type and direction, so the only thing
  // left to match is the target. Project to the target id — a predicate-shaped
  // projection would silently compare a bool against |node_id| (NodeId's
  // numeric ctor is non-explicit, so it compiles and never matches).
  auto refs = GetReferences(reference_type_id, forward);
  auto i = std::ranges::find(refs, node_id, [](const NodeRef::Reference& ref) {
    return ref.target.node_id();
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
      all_refs | std::views::transform(
                     [this](const scada::ReferenceDescription& desc) {
                       return service_.GetReference(desc);
                     });

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

NodeRef StaticNodeModel::GetChild(
    const scada::QualifiedName& child_name) const {
  // Mirrors v3 NodeModelImpl::GetChild: the hierarchical child whose browse
  // name matches (subtype matching in GetReferences covers HasProperty /
  // HasComponent / Organizes children).
  for (const auto& ref : GetReferences(
           scada::NodeId{scada::id::HierarchicalReferences}, true)) {
    if (ref.target.browse_name() == child_name)
      return ref.target;
  }
  return {};
}

scada::node StaticNodeModel::GetScadaNode() const {
  const auto node_id_attr = GetAttribute(scada::AttributeId::NodeId);
  const auto& node_id = node_id_attr.as_node_id();
  if (!node_id.is_null()) {
    return scada::client{service_.data_services_.as_services()}.node(node_id);
  } else {
    return {};
  }
}
