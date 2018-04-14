#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "core/configuration_types.h"
#include "core/data_value.h"

namespace scada {

class AddressSpace;
class Node;
class Variable;
class TypeDefinition;

QualifiedName GetBrowseName(AddressSpace& cfg, const NodeId& node_id);
LocalizedText GetDisplayName(AddressSpace& cfg, const NodeId& node_id);

enum class DataValueFieldId { Qualifier, Count };

NodeId NodeIdFromAliasedString(AddressSpace& address_space,
                               const base::StringPiece& path);
std::pair<NodeId, DataValueFieldId> ParseAliasedString(
    AddressSpace& address_space,
    const base::StringPiece& path);

const Node* GetTsFormat(const Node& ts_node);

bool IsSimulated(const Node& node, bool recursive);
bool IsDisabled(const Node& node, bool recursive);

Node* GetNestedNode(AddressSpace& address_space,
                    const NodeId& node_id,
                    base::StringPiece& nested_name);

base::string16 GetFullDisplayName(const Node& node);

scada::Status ConvertPropertyValues(scada::Node& node,
                                    scada::NodeProperties& properties);

}  // namespace scada
