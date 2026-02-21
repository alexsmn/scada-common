#include "node_service/node_util.h"

#include "base/strings/strcat.h"
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
    return base::StrCat(
        {GetFullDisplayName(parent), u" : ", ToString16(node.display_name())});
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
