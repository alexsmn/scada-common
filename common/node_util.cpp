#include "common/node_util.h"

#include "base/strings/utf_string_conversions.h"
#include "common/format.h"
#include "common/node_service.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"

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

bool CanCreate(const NodeRef& parent,
               const NodeRef& component_type_definition) {
  for (const auto& creates : parent.targets(scada::id::Creates)) {
    if (IsSubtypeOf(component_type_definition, creates.node_id()))
      return true;
  }

  for (auto type_definition = parent.type_definition(); type_definition;
       type_definition = type_definition.supertype()) {
    for (const auto& creates : type_definition.targets(scada::id::Creates)) {
      if (IsSubtypeOf(component_type_definition, creates.node_id()))
        return true;
    }
  }

  return false;
}

std::vector<NodeRef> GetDataVariables(const NodeRef& node) {
  std::vector<NodeRef> result;

  for (const auto& component : node.targets(scada::id::HasComponent)) {
    if (component.node_class() == scada::NodeClass::Variable)
      result.emplace_back(component);
  }

  return result;
}

base::string16 GetFullDisplayName(const NodeRef& node) {
  auto parent = node.parent();
  if (IsInstanceOf(parent, data_items::id::DataGroupType) ||
      IsInstanceOf(parent, devices::id::DeviceType))
    return GetFullDisplayName(parent) + base::WideToUTF16(L" : ") +
           ToString16(node.display_name());
  else
    return ToString16(node.display_name());
}

scada::LocalizedText GetDisplayName(NodeService& node_service,
                                    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return {};

  auto node = node_service.GetNode(node_id);
  return node ? node.display_name() : base::WideToUTF16(kUnknownDisplayName);
}
