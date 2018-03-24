#include "common/node_util.h"

#include "common/format.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"

base::string16 GetFullDisplayName(const NodeRef& node) {
  auto parent = node.parent();
  if (IsInstanceOf(parent, ::id::DataGroupType) ||
      IsInstanceOf(parent, ::id::DeviceType))
    return GetFullDisplayName(parent) + L" : " +
           ToString16(node.display_name());
  else
    return ToString16(node.display_name());
}

scada::LocalizedText GetDisplayName(NodeService& node_service,
                                    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return {};

  auto node = node_service.GetNode(node_id);
  return node ? node.display_name() : kUnknownDisplayName;
}
