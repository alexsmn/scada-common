#include "node_service/node_util.h"

#include "base/u16format.h"
#include "common/format.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"

bool IsSubtypeOf(NodeRef type_definition, const scada::NodeId& supertype_id) {
  for (; type_definition; type_definition = type_definition.supertype()) {
    if (type_definition.node_id() == supertype_id)
      return true;
  }
  return false;
}

bool IsInstanceOf(const NodeRef& node,
                  const scada::NodeId& type_definition_id) {
  return IsSubtypeOf(node.type_definition(), type_definition_id);
}

std::vector<NodeRef> GetCreatableChildTypes(const NodeRef& node) {
  std::vector<NodeRef> result;

  // Scan `node` itself when it is a type, else its type definition; walk the
  // supertype chain so inherited placeholders are included.
  NodeRef type =
      (node.node_class() && scada::IsTypeDefinition(*node.node_class()))
          ? node
          : node.type_definition();

  for (; type; type = type.supertype()) {
    // Placeholder children attach via a hierarchical reference (Organizes /
    // HasComponent subtype); need the type's children and each child fetched.
    type.StartFetch(NodeFetchStatus::NodeAndChildren);
    for (const auto& child : type.targets(scada::id::HierarchicalReferences)) {
      child.StartFetch(NodeFetchStatus::NodeOnly);
      const scada::NodeId rule_id =
          child.target(scada::id::HasModellingRule).node_id();
      if (rule_id !=
              scada::NodeId{scada::id::ModellingRule_OptionalPlaceholder} &&
          rule_id !=
              scada::NodeId{scada::id::ModellingRule_MandatoryPlaceholder})
        continue;
      if (NodeRef type_definition = child.type_definition())
        result.push_back(std::move(type_definition));
    }
  }
  return result;
}

std::vector<NodeRef> GetDataVariables(const NodeRef& node) {
  std::vector<NodeRef> result;

  for (const auto& component : node.targets(scada::id::HasComponent)) {
    if (component.node_class() == scada::NodeClass::Variable)
      result.emplace_back(component);
  }

  return result;
}

std::u16string GetFullDisplayName(const NodeRef& node) {
  auto parent = node.inverse_target(scada::id::Organizes);
  if (IsInstanceOf(parent, data_items::id::DataGroupType) ||
      IsInstanceOf(parent, devices::id::DeviceType))
    return u16format(L"{} : {}", GetFullDisplayName(parent),
                      ToString16(node.display_name()));
  else
    return ToString16(node.display_name());
}

scada::LocalizedText GetDisplayName(NodeService& node_service,
                                    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return {};

  auto node = node_service.GetNode(node_id);
  return node ? node.display_name() : scada::LocalizedText{kUnknownDisplayName};
}
