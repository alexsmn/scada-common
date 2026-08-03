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

// Materializes a type's HasComponent InstanceDeclarations onto an instance.
//
// Replaces CreateDataVariables, which assumed every component was a Variable
// with a type definition and hard-Checked both. That held only for as long as
// no SCADA type had an Object or Method component; UserType and RoleType carry
// Methods and Iec61850DeviceType carries an Object, so the assumption was a
// live crash waiting for the first caller pointed at one of them.
//
// Handles Variable, Object and Method declarations and skips anything else.
// Methods are created without a type definition (a Method node has none) and
// terminate the recursion; an Object component recurses through the factory so
// its own children are materialized in turn.
//
// Kept separate from CreateMissingProperties because the two differ in the
// reference type they attach with and in where the instance's type definition
// comes from — a property is always a PropertyType, a component takes the
// declaration's own.
scada::Status CreateMissingChildren(
    NodeFactory& node_factory,
    const scada::NodeId& node_id,
    const scada::TypeDefinition& type_definition);
