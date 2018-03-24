#pragma once

#include "common/node_ref.h"

class NodeService;

inline bool IsSubtypeOf(NodeRef type_definition,
                        const scada::NodeId& supertype_id) {
  for (; type_definition; type_definition = type_definition.supertype()) {
    if (type_definition.id() == supertype_id)
      return true;
  }
  return false;
}

inline bool IsInstanceOf(const NodeRef& node,
                         const scada::NodeId& type_definition_id) {
  return IsSubtypeOf(node.type_definition(), type_definition_id);
}

inline bool HasComponent(NodeRef parent_type, NodeRef component_type) {
  for (auto component_decl : parent_type.components()) {
    assert(component_decl.node_class() &&
           scada::IsInstance(*component_decl.node_class()));
    if (IsSubtypeOf(component_type, component_decl.type_definition().id()))
      return true;
  }
  return false;
}

inline std::vector<NodeRef> GetDataVariables(const NodeRef& node) {
  std::vector<NodeRef> result;

  for (auto& component : node.components()) {
    if (component.node_class() == scada::NodeClass::Variable)
      result.emplace_back(component);
  }

  return result;
}

base::string16 GetFullDisplayName(const NodeRef& node);

scada::LocalizedText GetDisplayName(NodeService& node_service,
                                    const scada::NodeId& node_id);
