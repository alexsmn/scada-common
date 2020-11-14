#include "address_space/address_space_util.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/format.h"
#include "base/string_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "common/format.h"
#include "core/view_service.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"

namespace scada {

namespace {

const Node* FindNodeByAlias(const Node& parent_node,
                            const std::string_view& alias) {
  for (auto* node : GetOrganizes(parent_node)) {
    if (IsInstanceOf(node, data_items::id::DataItemType)) {
      const auto& node_alias =
          GetPropertyValue(*node, data_items::id::DataItemType_Alias)
              .get_or(std::string());
      if (IsEqualNoCase(node_alias, alias))
        return node;
    }

    if (auto* child = FindNodeByAlias(*node, alias))
      return child;
  }

  return nullptr;
}

}  // namespace

NodeId NodeIdFromAliasedString(AddressSpace& address_space,
                               const std::string_view& path) {
  auto node_id = NodeIdFromScadaString(path);
  if (!node_id.is_null())
    return node_id;

  if (path.find_first_of('.') != std::string::npos)
    return {};

  auto* objects = address_space.GetNode(id::ObjectsFolder);
  assert(objects);
  if (!objects)
    return {};

  auto* node = FindNodeByAlias(*objects, path);
  return node ? node->id() : NodeId();
}

std::pair<NodeId, DataValueFieldId> ParseAliasedString(
    AddressSpace& address_space,
    const std::string_view& path) {
  std::string::size_type sep_pos = path.find_first_of('!');
  if (sep_pos == std::string::npos) {
    auto node_id = NodeIdFromAliasedString(address_space, path);
    if (node_id.is_null())
      return {};
    return {std::move(node_id), DataValueFieldId::Count};
  }

  auto str = path.substr(0, sep_pos);
  auto node_id = NodeIdFromAliasedString(address_space, str);
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

QualifiedName GetBrowseName(AddressSpace& cfg, const NodeId& node_id) {
  if (node_id.is_null())
    return {};
  auto* node = cfg.GetNode(node_id);
  return node ? node->GetBrowseName() : QualifiedName{};
}

LocalizedText GetDisplayName(AddressSpace& cfg, const NodeId& node_id) {
  auto* node = cfg.GetNode(node_id);
  return node ? GetFullDisplayName(*node)
              : base::WideToUTF16(kUnknownDisplayName);
}

std::wstring GetFullDisplayName(const Node& node) {
  auto* parent = GetParent(node);
  if (IsInstanceOf(parent, data_items::id::DataGroupType) ||
      IsInstanceOf(parent, devices::id::DeviceType))
    return GetFullDisplayName(*parent) + base::WideToUTF16(L" : ") +
           ToString16(node.GetDisplayName());
  else
    return ToString16(node.GetDisplayName());
}

Node* GetNestedNode(AddressSpace& address_space,
                    const NodeId& node_id,
                    std::string_view& nested_name) {
  nested_name = {};

  auto* node = address_space.GetNode(node_id);
  if (node)
    return node;

  NodeId parent_id;
  if (!IsNestedNodeId(node_id, parent_id, nested_name))
    parent_id = node_id;

  node = address_space.GetNode(parent_id);
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

Variant::Type DataTypeToValueType(const TypeDefinition& type) {
  if (type.id() == id::Boolean)
    return Variant::BOOL;
  else if (type.id() == id::Int16)
    return Variant::INT16;
  else if (type.id() == id::UInt16)
    return Variant::UINT16;
  else if (type.id() == id::Int32)
    return Variant::INT32;
  else if (type.id() == id::Double)
    return Variant::DOUBLE;
  else if (type.id() == id::String)
    return Variant::STRING;
  else if (type.id() == id::LocalizedText)
    return Variant::LOCALIZED_TEXT;
  else if (type.id() == id::NodeId)
    return Variant::NODE_ID;
  else if (auto* supertype = type.supertype())
    return DataTypeToValueType(*supertype);
  else
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

Status ConvertPropertyValues(Node& node, NodeProperties& properties) {
  auto* type = node.type_definition();
  assert(type);

  for (auto& prop : properties) {
    auto prop_decl_id = prop.first;
    auto* prop_decl =
        type ? AsVariable(GetAggregateDeclaration(*type, prop_decl_id))
             : nullptr;
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

bool WantsReference(AddressSpace& address_space,
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

ObjectType& BindObjectType(AddressSpace& address_space, const NodeId& node_id) {
  auto* node = scada::AsObjectType(address_space.GetNode(node_id));
  assert(node);
  return *node;
}

VariableType& BindVariableType(AddressSpace& address_space,
                               const NodeId& node_id) {
  auto* node = scada::AsVariableType(address_space.GetNode(node_id));
  assert(node);
  return *node;
}

}  // namespace scada
