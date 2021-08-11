#pragma once

namespace scada {
class NodeId;
class TypeDefinition;
}  // namespace scada

class NodeFactory;

// Throws an exception on error.
void CreateProperties(NodeFactory& node_factory,
                      const scada::NodeId& node_id,
                      const scada::TypeDefinition& type_definition);

void CreateDataVariables(NodeFactory& node_factory,
                         const scada::NodeId& node_id,
                         const scada::TypeDefinition& type_definition);
