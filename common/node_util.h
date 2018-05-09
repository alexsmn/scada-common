#pragma once

#include "common/node_ref.h"

class NodeService;

bool IsSubtypeOf(NodeRef type_definition, const scada::NodeId& supertype_id);

bool IsInstanceOf(const NodeRef& node, const scada::NodeId& type_definition_id);

bool CanCreate(const NodeRef& parent, const NodeRef& component_type_definition);

std::vector<NodeRef> GetDataVariables(const NodeRef& node);

base::string16 GetFullDisplayName(const NodeRef& node);

scada::LocalizedText GetDisplayName(NodeService& node_service,
                                    const scada::NodeId& node_id);
