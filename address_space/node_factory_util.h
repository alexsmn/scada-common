#pragma once

#include "scada/status.h"

namespace scada {
class NodeId;
class TypeDefinition;
}  // namespace scada

class NodeFactory;

// WARNING: This method generates artificial IDs for property nodes.
scada::Status CreateMissingProperties(
    NodeFactory& node_factory,
    const scada::NodeId& node_id,
    const scada::TypeDefinition& type_definition);

scada::Status CreateDataVariables(
    NodeFactory& node_factory,
    const scada::NodeId& node_id,
    const scada::TypeDefinition& type_definition);
