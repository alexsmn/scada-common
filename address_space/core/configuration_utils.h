#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "core/configuration_types.h"
#include "core/data_value.h"

namespace scada {

class Configuration;
class Node;
class Variable;
class TypeDefinition;

QualifiedName GetBrowseName(Configuration& cfg, const NodeId& node_id);
LocalizedText GetDisplayName(Configuration& cfg, const NodeId& node_id);

enum class DataValueFieldId { Qualifier, Count };

NodeId NodeIdFromAliasedString(Configuration& address_space,
                               const base::StringPiece& path);
std::pair<NodeId, DataValueFieldId> ParseAliasedString(
    Configuration& address_space,
    const base::StringPiece& path);

const Node* GetTsFormat(const Node& ts_node);

bool IsSimulated(const Node& node, bool recursive);
bool IsDisabled(const Node& node, bool recursive);

Node* GetNestedNode(Configuration& address_space,
                    const NodeId& node_id,
                    base::StringPiece& nested_name);

base::string16 GetFullDisplayName(const Node& node);

}  // namespace scada
