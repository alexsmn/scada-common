#include "address_space/address_space_util.h"

#include "address_space/address_space.h"
#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/node.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/format.h"
#include "base/string_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "common/format.h"
#include "common/node_state.h"
#include "core/view_service.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"

namespace scada {

std::pair<NodeId, DataValueFieldId> ParseDataValueFieldString(
    const std::string_view& path) {
  std::string::size_type sep_pos = path.find_first_of('!');
  if (sep_pos == std::string::npos) {
    auto node_id = NodeIdFromScadaString(path);
    if (node_id.is_null())
      return {};
    return {std::move(node_id), DataValueFieldId::Count};
  }

  auto str = path.substr(0, sep_pos);
  auto node_id = NodeIdFromScadaString(str);
  if (node_id.is_null())
    return {};

  auto property_name = path.substr(sep_pos + 1);
  if (IsEqualNoCase(property_name, "Quality"))
    return {std::move(node_id), DataValueFieldId::Qualifier};
  else if (IsEqualNoCase(property_name, "ServerTimeStamp"))
    return {std::move(node_id), DataValueFieldId::ServerTimeStamp};
  else if (IsEqualNoCase(property_name, "SourceTimeStamp"))
    return {std::move(node_id), DataValueFieldId::SourceTimeStamp};

  return {};
}

bool IsSimulated(const Node& node, bool recursive) {
  bool simulated = false;
  if (IsInstanceOf(&node, data_items::id::DataGroupType)) {
    simulated = GetPropertyValue(node, data_items::id::DataGroupType_Simulated)
                    .get_or(false);
  } else if (IsInstanceOf(&node, data_items::id::DataItemType)) {
    simulated = GetPropertyValue(node, data_items::id::DataItemType_Simulated)
                    .get_or(false);
  }
  if (simulated)
    return true;

  if (!recursive)
    return false;

  if (auto* parent = GetParent(node))
    return IsSimulated(*parent, true);

  return false;
}

bool IsDisabled(const Node& node, bool recursive) {
  if (!IsInstanceOf(&node, devices::id::DeviceType))
    return false;

  bool disabled =
      GetPropertyValue(node, devices::id::DeviceType_Disabled).get_or(false);
  if (disabled)
    return true;

  if (!recursive)
    return false;

  if (auto* parent = GetParent(node))
    return IsDisabled(*parent, true);

  return false;
}

const Node* GetTsFormat(const Node& ts_node) {
  return GetReference(ts_node, data_items::id::HasTsFormat).node;
}

QualifiedName GetBrowseName(const AddressSpace& address_space,
                            const NodeId& node_id) {
  if (node_id.is_null())
    return {};
  auto* node = address_space.GetNode(node_id);
  return node ? node->GetBrowseName() : QualifiedName{};
}

LocalizedText GetDisplayName(const AddressSpace& address_space,
                             const NodeId& node_id) {
  auto* node = address_space.GetNode(node_id);
  return node ? GetFullDisplayName(*node) : LocalizedText{kUnknownDisplayName};
}

std::u16string GetFullDisplayName(const Node& node) {
  auto* parent = GetParent(node);
  if (IsInstanceOf(parent, data_items::id::DataGroupType) ||
      IsInstanceOf(parent, devices::id::DeviceType))
    return base::StrCat({GetFullDisplayName(*parent), u" : ",
                         ToString16(node.GetDisplayName())});
  else
    return ToString16(node.GetDisplayName());
}

Node* GetMutableNestedNode(AddressSpace& address_space,
                           const NodeId& node_id,
                           std::string_view& nested_name) {
  nested_name = {};

  auto* node = address_space.GetMutableNode(node_id);
  if (node)
    return node;

  NodeId parent_id;
  if (!IsNestedNodeId(node_id, parent_id, nested_name))
    parent_id = node_id;

  node = address_space.GetMutableNode(parent_id);
  if (!node)
    return nullptr;

  while (!nested_name.empty()) {
    auto p = nested_name.find('!');
    if (p == std::string_view::npos)
      p = nested_name.size();

    auto next_nested_part = nested_name.substr(0, p);
    auto* child = FindChild(*node, next_nested_part);
    if (!child)
      break;

    node = child;
    nested_name = nested_name.substr(p + 1);
  }

  return node;
}

const Node* GetNestedNode(const AddressSpace& address_space,
                          const NodeId& node_id,
                          std::string_view& nested_name) {
  return GetMutableNestedNode(const_cast<AddressSpace&>(address_space), node_id,
                              nested_name);
}

Variant::Type DataTypeToValueType(const TypeDefinition& type) {
  for (auto* supertype = &type; supertype; supertype = supertype->supertype()) {
    auto build_in_data_type = ToBuiltInDataType(supertype->id());
    if (build_in_data_type != Variant::COUNT)
      return build_in_data_type;
  }

  return Variant::EMPTY;
}

Status ConvertPropertyValue(const DataType& data_type, Variant& value) {
  if (!value.is_null()) {
    if (data_type.id() != id::BaseDataType) {
      auto value_type = DataTypeToValueType(data_type);
      assert(value_type != Variant::EMPTY);
      if (!value.ChangeType(value_type)) {
        assert(false);
        return StatusCode::Bad;
      }
    }
  }

  return StatusCode::Good;
}

StatusCode ConvertPropertyValues(const TypeDefinition& type_definition,
                                 NodeProperties& properties) {
  for (auto& prop : properties) {
    auto& prop_decl_id = prop.first;
    auto* prop_decl =
        AsVariable(GetAggregateDeclaration(type_definition, prop_decl_id));
    if (!prop_decl) {
      assert(false);
      return StatusCode::Bad_WrongPropertyId;
    }

    auto& value = prop.second;
    auto status = ConvertPropertyValue(prop_decl->GetDataType(), value);
    if (!status) {
      assert(false);
      return StatusCode::Bad;
    }
  }

  return StatusCode::Good;
}

bool WantsTypeDefinition(AddressSpace& address_space,
                         const BrowseDescription& description) {
  return WantsReference(address_space, description, id::HasTypeDefinition,
                        true);
}

bool WantsOrganizes(AddressSpace& address_space,
                    const BrowseDescription& description) {
  return WantsReference(address_space, description, id::Organizes, true);
}

bool WantsParent(AddressSpace& address_space,
                 const BrowseDescription& description) {
  return WantsReference(address_space, description, id::HierarchicalReferences,
                        false);
}

bool WantsReference(const AddressSpace& address_space,
                    const BrowseDescription& description,
                    const NodeId& reference_type_id,
                    bool forward) {
  if (description.direction != BrowseDirection::Both) {
    const bool wants_forward =
        description.direction == BrowseDirection::Forward;
    if (forward != wants_forward)
      return false;
  }

  if (description.include_subtypes) {
    return IsSubtypeOf(address_space, reference_type_id,
                       description.reference_type_id);
  } else {
    return reference_type_id == description.reference_type_id;
  }
}

const ReferenceType& BindReferenceType(const AddressSpace& address_space,
                                       const NodeId& node_id) {
  auto* node = AsReferenceType(address_space.GetNode(node_id));
  assert(node);
  return *node;
}

const ObjectType& BindObjectType(const AddressSpace& address_space,
                                 const NodeId& node_id) {
  auto* node = AsObjectType(address_space.GetNode(node_id));
  assert(node);
  return *node;
}

const VariableType& BindVariableType(const AddressSpace& address_space,
                                     const NodeId& node_id) {
  auto* node = AsVariableType(address_space.GetNode(node_id));
  assert(node);
  return *node;
}

bool IsSubtypeOf(const AddressSpace& address_space,
                 const NodeId& type_id,
                 const NodeId& supertype_id) {
  const auto* type = AsTypeDefinition(address_space.GetNode(type_id));
  for (; type; type = type->supertype()) {
    if (type->id() == supertype_id)
      return true;
  }
  return false;
}

void AddReference(MutableAddressSpace& address_space,
                  const NodeId& reference_type_id,
                  const NodeId& source_id,
                  const NodeId& target_id) {
  auto* source = address_space.GetMutableNode(source_id);
  assert(source);

  auto* target = address_space.GetMutableNode(target_id);
  assert(target);

  AddReference(address_space, reference_type_id, *source, *target);
}

void AddReference(MutableAddressSpace& address_space,
                  const NodeId& reference_type_id,
                  Node& source,
                  Node& target) {
  auto* reference_type =
      AsReferenceType(address_space.GetNode(reference_type_id));
  assert(reference_type);
  address_space.AddReference(*reference_type, source, target);
}

void DeleteReference(MutableAddressSpace& address_space,
                     const NodeId& reference_type_id,
                     const NodeId& source_id,
                     const NodeId& target_id) {
  auto* reference_type =
      AsReferenceType(address_space.GetNode(reference_type_id));
  assert(reference_type);

  auto* source = address_space.GetMutableNode(source_id);
  assert(source);

  auto* target = address_space.GetMutableNode(target_id);
  assert(target);

  address_space.DeleteReference(*reference_type, *source, *target);
}

void DeleteAllReferences(MutableAddressSpace& address_space, Node& node) {
  while (!node.inverse_references().empty()) {
    auto ref = node.inverse_references().back();
    address_space.DeleteReference(*ref.type, *ref.node, node);
  }
  while (!node.forward_references().empty()) {
    auto ref = node.forward_references().back();
    address_space.DeleteReference(*ref.type, node, *ref.node);
  }
}

}  // namespace scada

void SortNodesHierarchically(std::vector<scada::NodeState>& nodes) {
  struct Visitor {
    Visitor(std::vector<scada::NodeState>& nodes)
        : nodes{nodes}, visited(nodes.size(), false) {
      for (size_t index = 0; index < nodes.size(); ++index)
        map.emplace(nodes[index].node_id, index);

      sorted_indexes.reserve(nodes.size());
      for (size_t index = 0; index < nodes.size(); ++index)
        VisitIndex(index);

      assert(sorted_indexes.size() == nodes.size());

      std::vector<scada::NodeState> sorted_nodes(sorted_indexes.size());
      for (size_t index = 0; index < nodes.size(); ++index)
        sorted_nodes[index] = std::move(nodes[sorted_indexes[index]]);
      nodes = std::move(sorted_nodes);
    }

    void Visit(const scada::NodeId& node_id) {
      if (node_id.is_null())
        return;
      auto i = map.find(node_id);
      if (i != map.end())
        VisitIndex(i->second);
    }

    void VisitIndex(size_t index) {
      if (visited[index])
        return;

      visited[index] = true;

      auto& node = nodes[index];
      if (!node.supertype_id.is_null())
        Visit(scada::id::HasSubtype);
      Visit(node.supertype_id);
      Visit(node.reference_type_id);
      Visit(node.parent_id);
      Visit(node.type_definition_id);
      Visit(node.attributes.data_type);

      sorted_indexes.emplace_back(index);
    }

    std::vector<scada::NodeState>& nodes;
    std::vector<bool> visited;
    std::map<scada::NodeId, size_t /*index*/> map;
    std::vector<size_t /*indexes*/> sorted_indexes;
  };

  if (nodes.size() >= 2)
    Visitor{nodes};
}
