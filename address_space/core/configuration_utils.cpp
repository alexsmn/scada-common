#include "core/configuration_utils.h"

#include "base/format.h"
#include "base/string_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "common/format.h"
#include "common/node_id_util.h"
#include "common/scada_node_ids.h"
#include "core/configuration.h"
#include "core/configuration_utils.h"
#include "core/node.h"
#include "core/node_utils.h"
#include "core/type_definition.h"
#include "core/variable.h"

namespace scada {

namespace {

const Node* FindNodeByAlias(const Node& parent_node,
                            const base::StringPiece& alias) {
  for (auto* node : GetOrganizes(parent_node)) {
    auto& node_alias =
        GetPropertyValue(*node, ::id::DataItemType_Alias).get_or(std::string());
    if (IsEqualNoCase(node_alias, alias))
      return node;
    if (auto* child = FindNodeByAlias(*node, alias))
      return child;
  }
  return nullptr;
}

}  // namespace

NodeId NodeIdFromAliasedString(Configuration& cfg,
                               const base::StringPiece& path) {
  auto node_id = NodeIdFromScadaString(path);
  if (!node_id.is_null())
    return node_id;

  if (path.find_first_of('.') != std::string::npos)
    return NodeId();

  auto* node = FindNodeByAlias(*cfg.GetNode(id::RootFolder), path);
  return node ? node->id() : NodeId();
}

std::pair<NodeId, DataValueFieldId> ParseAliasedString(
    Configuration& address_space,
    const base::StringPiece& path) {
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
  return {};
}

bool IsSimulated(const Node& node, bool recursive) {
  bool simulated = false;
  if (IsInstanceOf(&node, ::id::DataGroupType)) {
    simulated =
        GetPropertyValue(node, ::id::DataGroupType_Simulated).get_or(false);
  } else if (IsInstanceOf(&node, ::id::DataItemType)) {
    simulated =
        GetPropertyValue(node, ::id::DataItemType_Simulated).get_or(false);
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
  bool disabled =
      GetPropertyValue(node, ::id::DeviceType_Disabled).get_or(false);
  if (disabled)
    return true;

  if (!recursive)
    return false;

  if (auto* parent = GetParent(node))
    return IsDisabled(*parent, true);

  return false;
}

const Node* GetTsFormat(const Node& ts_node) {
  return GetReference(ts_node, ::id::HasTsFormat).node;
}

QualifiedName GetBrowseName(Configuration& cfg, const NodeId& node_id) {
  if (node_id.is_null())
    return {};
  auto* node = cfg.GetNode(node_id);
  return node ? node->GetBrowseName() : QualifiedName{};
}

LocalizedText GetDisplayName(Configuration& cfg, const NodeId& node_id) {
  auto* node = cfg.GetNode(node_id);
  return node ? GetFullDisplayName(*node) : kUnknownDisplayName;
}

base::string16 GetFullDisplayName(const Node& node) {
  auto* parent = GetParent(node);
  if (IsInstanceOf(parent, ::id::DataGroupType) ||
      IsInstanceOf(parent, ::id::DeviceType))
    return GetFullDisplayName(*parent) + L" : " +
           ToString16(node.GetDisplayName());
  else
    return ToString16(node.GetDisplayName());
}

Node* GetNestedNode(Configuration& address_space,
                    const NodeId& node_id,
                    base::StringPiece& nested_name) {
  nested_name.clear();

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
    if (p == base::StringPiece::npos)
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

scada::Variant::Type DataTypeToValueType(const scada::TypeDefinition& type) {
  if (type.id() == scada::id::Boolean)
    return scada::Variant::BOOL;
  else if (type.id() == scada::id::Int32)
    return scada::Variant::INT32;
  else if (type.id() == scada::id::Double)
    return scada::Variant::DOUBLE;
  else if (type.id() == scada::id::String)
    return scada::Variant::STRING;
  else if (type.id() == scada::id::LocalizedText)
    return scada::Variant::LOCALIZED_TEXT;
  else if (type.id() == scada::id::NodeId)
    return scada::Variant::NODE_ID;
  else if (auto* supertype = GetSupertype(type))
    return DataTypeToValueType(*supertype);
  else
    return scada::Variant::EMPTY;
}

scada::Status ConvertPropertyValue(const scada::DataType& data_type,
                                   scada::Variant& value) {
  if (!value.is_null()) {
    if (data_type.id() != scada::id::BaseDataType) {
      auto value_type = DataTypeToValueType(data_type);
      if (!value.ChangeType(value_type)) {
        assert(false);
        return scada::StatusCode::Bad;
      }
    }
  }

  return scada::StatusCode::Good;
}

scada::Status ConvertPropertyValues(scada::Node& node,
                                    scada::NodeProperties& properties) {
  auto* type = scada::GetTypeDefinition(node);
  assert(type);

  for (auto& prop : properties) {
    auto prop_decl_id = prop.first;
    auto* prop_decl =
        type ? scada::AsVariable(GetAggregateDeclaration(*type, prop_decl_id))
             : nullptr;
    if (!prop_decl) {
      assert(false);
      return scada::StatusCode::Bad_WrongPropertyId;
    }

    auto& value = prop.second;
    auto status = ConvertPropertyValue(prop_decl->GetDataType(), value);
    if (!status) {
      assert(false);
      return scada::StatusCode::Bad;
    }
  }

  return scada::StatusCode::Good;
}

}  // namespace scada
