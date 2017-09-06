#include "node_ref_util.h"

#include "common/browse_util.h"
#include "common/scada_node_ids.h"

bool IsSubtypeOf(const NodeRef& node, const scada::NodeId& type_id) {
  if (!node)
    return false;
  for (auto n = node; n; n = n.supertype()) {
    if (n.id() == type_id)
      return true;
  }
  return false;
}

bool IsInstanceOf(const NodeRef& node, const scada::NodeId& type_id) {
  if (!node)
    return false;
  return IsSubtypeOf(node.type_definition(), type_id);
}

void BrowseNodesRecursive(NodeRefService& service, const scada::NodeId& parent_id,
    const scada::NodeId& type_definition_id, const NodesCallback& callback) {
  struct Browser : public std::enable_shared_from_this<Browser> {
    Browser(NodeRefService& service, const scada::NodeId& type_definition_id, NodesCallback callback)
        : service{service},
          type_definition_id_{type_definition_id},
          callback{std::move(callback)} {
    }

    Browser(const Browser&) = delete;

    ~Browser() { callback(std::move(nodes)); }

    void Browse(const scada::NodeId& parent_id) {
      auto self = shared_from_this();
      BrowseNodes(service, {parent_id, scada::BrowseDirection::Forward, OpcUaId_HierarchicalReferences, true},
          [self, this](const scada::Status& status, const std::vector<NodeRef>& nodes) {
            for (auto& node : nodes) {
              if (type_definition_id_.is_null() || IsInstanceOf(node, type_definition_id_))
                this->nodes.emplace_back(node);
              Browse(node.id());
            }
          });
    }

    NodeRefService& service;
    NodesCallback callback;
    const scada::NodeId type_definition_id_;
    std::vector<NodeRef> nodes;
  };

  std::make_shared<Browser>(service, type_definition_id, callback)->Browse(parent_id);
}

void BrowseAllDevices(NodeRefService& service, const NodesCallback& callback) {
  BrowseNodesRecursive(service, id::Devices, id::DeviceType, [callback](std::vector<NodeRef> devices) {
    callback(std::move(devices));
  });
}

void BrowseInstanceDeclarations(NodeRefService& node_service, const scada::NodeId& type_definition_id,
      const scada::NodeId& reference_type_id, const NodesCallback& callback) {
  // Request this type's variable declarations.
  BrowseNodes(node_service, {type_definition_id, scada::BrowseDirection::Forward, reference_type_id, true},
      [&node_service, type_definition_id, callback, reference_type_id]
      (const scada::Status& status, std::vector<NodeRef> declarations) {
        auto declarations_ptr = std::make_shared<std::vector<NodeRef>>(std::move(declarations));
        // Request supertype.
        BrowseReference(node_service, {type_definition_id, scada::BrowseDirection::Inverse, OpcUaId_HasSubtype, true},
            [&node_service, callback, declarations_ptr, reference_type_id]
            (const scada::Status& status, const scada::ReferenceDescription& reference) {
              if (!status) {
                callback(std::move(*declarations_ptr));
                return;
              }
              // Request supertype's variables.
              const auto& supertype_id = reference.node_id;
              BrowseInstanceDeclarations(node_service, supertype_id, reference_type_id,
                  [declarations_ptr, callback](std::vector<NodeRef> super_declarations) {
                    declarations_ptr->insert(declarations_ptr->begin(), super_declarations.begin(), super_declarations.end());
                    callback(std::move(*declarations_ptr));
                  });
            });
      });
}
