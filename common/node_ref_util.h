#pragma once

#include "common/node_ref.h"
#include "core/standard_node_ids.h"

class NodeRefService;

bool IsSubtypeOf(const NodeRef& node, const scada::NodeId& type_id);
bool IsInstanceOf(const NodeRef& node, const scada::NodeId& type_id);

using NodesCallback = std::function<void(std::vector<NodeRef> nodes)>;
void BrowseNodesRecursive(NodeRefService& service, const scada::NodeId& parent_id,
    const scada::NodeId& type_definition_id, const NodesCallback& callback);

void BrowseAllDevices(NodeRefService& service, const NodesCallback& callback);

void BrowseInstanceDeclarations(NodeRefService& node_service, const scada::NodeId& type_definition_id,
    const scada::NodeId& reference_type_id, const NodesCallback& callback);

// Callback = void(std::vector<scada::NodeId>)
template<class Callback>
inline void BrowseSupertypeIds(NodeRefService& service, const scada::NodeId& type_definition_id, const Callback& callback) {
  BrowseReference(service, type_definition_id, OpcUaId_HasSubtype, false,
      [&service, callback](const scada::Status& status, const NodeRefService::ReferenceDescription& reference) {
        const auto& supertype_id = reference.reference_type_id;
        if (!status) {
          callback({supertype_id});
          return;
        }
        BrowseSupertypeIds(service, supertype_id,
            [supertype_id, callback](std::vector<scada::NodeId> supertype_ids) {
              supertype_ids.emplace_back(supertype_id);
              callback(std::move(supertype_ids));
            });
      });
}
