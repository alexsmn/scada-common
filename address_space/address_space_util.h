#pragma once

#include "core/configuration_types.h"
#include "core/data_value.h"

#include <cassert>
#include <string>
#include <vector>

class MutableAddressSpace;

namespace scada {

class AddressSpace;
class Node;
class ObjectType;
class ReferenceType;
class TypeDefinition;
class Variable;
class VariableType;
struct BrowseDescription;
struct NodeState;

QualifiedName GetBrowseName(const AddressSpace& address_space,
                            const NodeId& node_id);
LocalizedText GetDisplayName(const AddressSpace& address_space,
                             const NodeId& node_id);

enum class DataValueFieldId {
  Qualifier,
  SourceTimeStamp,
  ServerTimeStamp,
  Count
};

std::pair<NodeId, DataValueFieldId> ParseDataValueFieldString(
    const std::string_view& path);

const Node* GetTsFormat(const Node& ts_node);

bool IsSimulated(const Node& node, bool recursive);
bool IsDisabled(const Node& node, bool recursive);

Node* GetMutableNestedNode(AddressSpace& address_space,
                           const NodeId& node_id,
                           std::string_view& nested_name);
const Node* GetNestedNode(const AddressSpace& address_space,
                          const NodeId& node_id,
                          std::string_view& nested_name);

std::u16string GetFullDisplayName(const Node& node);

StatusCode ConvertPropertyValues(const TypeDefinition& type_definition,
                                 NodeProperties& properties);

bool WantsReference(const AddressSpace& address_space,
                    const BrowseDescription& description,
                    const NodeId& reference_type_id,
                    bool forward);
bool WantsTypeDefinition(AddressSpace& address_space,
                         const BrowseDescription& description);
bool WantsOrganizes(AddressSpace& address_space,
                    const BrowseDescription& description);
bool WantsParent(AddressSpace& address_space,
                 const BrowseDescription& description);

const ReferenceType& BindReferenceType(const AddressSpace& address_space,
                                       const NodeId& node_id);
const ObjectType& BindObjectType(const AddressSpace& address_space,
                                 const NodeId& node_id);
const VariableType& BindVariableType(const AddressSpace& address_space,
                                     const NodeId& node_id);

bool IsSubtypeOf(const AddressSpace& address_space,
                 const NodeId& type_id,
                 const NodeId& supertype_id);

void AddReference(MutableAddressSpace& address_space,
                  const NodeId& reference_type_id,
                  const NodeId& source_id,
                  const NodeId& target_id);
void AddReference(MutableAddressSpace& address_space,
                  const NodeId& reference_type_id,
                  Node& source,
                  Node& target);

void DeleteReference(MutableAddressSpace& address_space,
                     const NodeId& reference_type_id,
                     const NodeId& source_id,
                     const NodeId& target_id);

void DeleteAllReferences(MutableAddressSpace& address_space, Node& node);

}  // namespace scada

void SortNodesHierarchically(std::vector<scada::NodeState>& nodes);
