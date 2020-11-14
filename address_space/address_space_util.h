#pragma once

#include "core/configuration_types.h"
#include "core/data_value.h"

#include <cassert>
#include <string>
#include <vector>

namespace scada {

class AddressSpace;
class Node;
class ObjectType;
class TypeDefinition;
class Variable;
class VariableType;
struct BrowseDescription;
struct NodeState;

QualifiedName GetBrowseName(AddressSpace& cfg, const NodeId& node_id);
LocalizedText GetDisplayName(AddressSpace& cfg, const NodeId& node_id);

enum class DataValueFieldId {
  Qualifier,
  SourceTimeStamp,
  ServerTimeStamp,
  Count
};

NodeId NodeIdFromAliasedString(AddressSpace& address_space,
                               const std::string_view& path);
std::pair<NodeId, DataValueFieldId> ParseAliasedString(
    AddressSpace& address_space,
    const std::string_view& path);

const Node* GetTsFormat(const Node& ts_node);

bool IsSimulated(const Node& node, bool recursive);
bool IsDisabled(const Node& node, bool recursive);

Node* GetNestedNode(AddressSpace& address_space,
                    const NodeId& node_id,
                    std::string_view& nested_name);

std::wstring GetFullDisplayName(const Node& node);

Status ConvertPropertyValues(Node& node, NodeProperties& properties);

bool WantsReference(AddressSpace& address_space,
                    const BrowseDescription& description,
                    const NodeId& reference_type_id,
                    bool forward);
bool WantsTypeDefinition(AddressSpace& address_space,
                         const BrowseDescription& description);
bool WantsOrganizes(AddressSpace& address_space,
                    const BrowseDescription& description);
bool WantsParent(AddressSpace& address_space,
                 const BrowseDescription& description);

ObjectType& BindObjectType(AddressSpace& address_space, const NodeId& node_id);
VariableType& BindVariableType(AddressSpace& address_space,
                               const NodeId& node_id);

}  // namespace scada

void SortNodesHierarchically(std::vector<scada::NodeState>& nodes);