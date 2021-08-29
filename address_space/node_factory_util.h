#pragma once

namespace scada {
class NodeId;
class TypeDefinition;
}  // namespace scada

class NodeFactory;

// WARNING: This method generates artificial IDs for property nodes.
// Throws an exception on error.
void CreateMissingProperties(NodeFactory& node_factory,
                             const scada::NodeId& node_id,
                             const scada::TypeDefinition& type_definition);

void CreateDataVariables(NodeFactory& node_factory,
                         const scada::NodeId& node_id,
                         const scada::TypeDefinition& type_definition);
