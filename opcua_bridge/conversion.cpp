#include "opcua_bridge/conversion.h"

#include "opcua_bridge/vector_conversion.h"
#include "scada/authorization.h"
#include "scada/identity_mapping_rule_encoding.h"
#include "scada/range_encoding.h"
#include "scada/user_management_encoding.h"

#include "opcua/transport/binary/codec_utils.h"
#include "opcua/ua/ua_binary_codec.h"
#include "opcua/ua/ua_types.h"

#include <any>
#include <cstdint>
#include <optional>
#include <vector>

namespace scada::opcua_bridge {

namespace {

// Serializes a GENERATED OPC UA structure into an ExtensionObject body, tagged
// with that type's own DefaultBinary encoding id. Both the field order and the
// id come from the vendored schema via tools/gen_ua_types.py — this file never
// restates either, so a schema bump is the only way the wire format can
// change. OPC UA Part 6 §5.1.8 ExtensionObject,
// https://reference.opcfoundation.org/Core/Part6/v105/docs/5.1.8
template <class T>
opcua::ExtensionObject EncodeGenerated(const T& value) {
  std::vector<char> body;
  opcua::binary::Encoder encoder{body};
  opcua::ua::Encode(encoder, value);
  return opcua::ExtensionObject{
      opcua::ExpandedNodeId{
          opcua::NodeId{opcua::ua::BinaryEncodingId<T>::value, 0}},
      std::any{std::move(body)}};
}

// The inverse: decodes `e` as T when it carries T's DefaultBinary encoding id,
// else nullopt. The wire decoder (ReadExtensionObjectValue) delivers the body
// as an opcua::ByteString (std::vector<char>), which is what EncodeGenerated
// produces in-process too, so both paths decode identically.
template <class T>
std::optional<T> DecodeGenerated(const opcua::ExtensionObject& e) {
  if (e.data_type_id().node_id() !=
      opcua::NodeId{opcua::ua::BinaryEncodingId<T>::value, 0}) {
    return std::nullopt;
  }
  const auto* body = std::any_cast<opcua::ByteString>(&e.value());
  if (!body) {
    return std::nullopt;
  }
  opcua::binary::Decoder decoder{*body};
  T value;
  if (!opcua::ua::Decode(decoder, value)) {
    return std::nullopt;
  }
  return value;
}

// Scalar-or-array conversion for an "identity" element type T (the same std
// type on both sides: bool, the numeric primitives, String, ByteString). The
// opcua Variant accepts the value directly.
template <class T>
opcua::Variant ToOpcuaSame(const scada::Variant& v) {
  if (v.is_array())
    return opcua::Variant{v.get<std::vector<T>>()};
  return opcua::Variant{v.get<T>()};
}
template <class T>
scada::Variant ToScadaSame(const opcua::Variant& v) {
  if (v.is_array())
    return scada::Variant{v.get<std::vector<T>>()};
  return scada::Variant{v.get<T>()};
}

// Scalar-or-array conversion for a class element type that must be converted.
template <class T>
opcua::Variant ToOpcuaConv(const scada::Variant& v) {
  if (v.is_array())
    return opcua::Variant{ToOpcuaVector(v.get<std::vector<T>>())};
  return opcua::Variant{ToOpcua(v.get<T>())};
}
template <class T>
scada::Variant ToScadaConv(const opcua::Variant& v) {
  if (v.is_array())
    return scada::Variant{ToScadaVector(v.get<std::vector<T>>())};
  return scada::Variant{ToScada(v.get<T>())};
}

}  // namespace

opcua::NodeId ToOpcua(const scada::NodeId& n) {
  switch (n.type()) {
    case scada::NodeIdType::Numeric:
      return opcua::NodeId{n.numeric_id(), n.namespace_index()};
    case scada::NodeIdType::String:
      return opcua::NodeId{n.string_id(), n.namespace_index()};
    case scada::NodeIdType::Opaque:
      return opcua::NodeId{n.opaque_id(), n.namespace_index()};
  }
  return {};
}

scada::NodeId ToScada(const opcua::NodeId& n) {
  switch (n.type()) {
    case opcua::NodeIdType::Numeric:
      return scada::NodeId{n.numeric_id(), n.namespace_index()};
    case opcua::NodeIdType::String:
      return scada::NodeId{n.string_id(), n.namespace_index()};
    case opcua::NodeIdType::Opaque:
      return scada::NodeId{n.opaque_id(), n.namespace_index()};
  }
  return {};
}

opcua::ExpandedNodeId ToOpcua(const scada::ExpandedNodeId& e) {
  return opcua::ExpandedNodeId{ToOpcua(e.node_id()), e.namespace_uri(),
                               e.server_index()};
}
scada::ExpandedNodeId ToScada(const opcua::ExpandedNodeId& e) {
  return scada::ExpandedNodeId{ToScada(e.node_id()), e.namespace_uri(),
                               e.server_index()};
}

opcua::QualifiedName ToOpcua(const scada::QualifiedName& q) {
  return opcua::QualifiedName{q.name(), q.namespace_index()};
}
scada::QualifiedName ToScada(const opcua::QualifiedName& q) {
  return scada::QualifiedName{q.name(), q.namespace_index()};
}

opcua::ExtensionObject ToOpcua(const scada::ExtensionObject& e) {
  // Each branch maps the transport-neutral scada:: struct onto its generated
  // opcua::ua:: counterpart and lets the GENERATED codec write the bytes. The
  // field order and the DefaultBinary encoding id both come from the vendored
  // schema through tools/gen_ua_types.py, so neither is ever restated here —
  // a wrong field order is fixed by bumping the schema, not by editing this
  // file. Part 6 §5.1.8 ExtensionObject: the wire codec
  // (AppendExtensionObjectValue) writes the std::vector<char> body verbatim.
  if (const auto* role = std::any_cast<scada::RolePermissionType>(&e.value())) {
    // OPC UA Part 3 §8.56 RolePermissionType.
    return EncodeGenerated(opcua::ua::RolePermissionType{
        .role_id = ToOpcua(role->role_id),
        .permissions =
            static_cast<opcua::ua::PermissionType>(role->permissions)});
  }

  // A Role's Identities entry, or an AddIdentity/RemoveIdentity argument.
  // OPC UA Part 18 §4.4.3,
  // https://reference.opcfoundation.org/Core/Part18/v105/docs/4.4.3
  if (const auto* rule =
          std::any_cast<scada::IdentityMappingRule>(&e.value())) {
    return EncodeGenerated(opcua::ua::IdentityMappingRuleType{
        .criteria_type =
            static_cast<opcua::ua::IdentityCriteriaType>(rule->criteria_type),
        .criteria = rule->criteria});
  }

  // One entry of the UserManagement object's Users property. OPC UA Part 18
  // §5.2.4, https://reference.opcfoundation.org/Core/Part18/v105/docs/5.2.4
  if (const auto* user =
          std::any_cast<scada::UserManagementDataType>(&e.value())) {
    return EncodeGenerated(opcua::ua::UserManagementDataType{
        .user_name = user->user_name,
        .user_configuration = static_cast<opcua::ua::UserConfigurationMask>(
            user->user_configuration),
        .description = user->description});
  }

  // The UserManagement object's PasswordLength (OPC UA Part 18 §5.2.2), a
  // Range of Part 8 §5.6.2.
  if (const auto* range = std::any_cast<scada::Range>(&e.value())) {
    return EncodeGenerated(
        opcua::ua::Range{.low = range->low, .high = range->high});
  }

  // Other payloads cannot be transferred across the type boundary by value; the
  // boundary ExtensionObject then carries only its data_type_id.
  return opcua::ExtensionObject{ToOpcua(e.data_type_id()), {}};
}
scada::ExtensionObject ToScada(const opcua::ExtensionObject& e) {
  // The mirror of ToOpcua: the GENERATED codec reads the body, and each branch
  // maps the generated struct back onto its transport-neutral counterpart, so
  // method implementations receive structured arguments (AddIdentity /
  // RemoveIdentity, OPC UA Part 18 §4.4.5/§4.4.6) and a client reading Users
  // receives accounts rather than opaque bytes. DecodeGenerated matches on the
  // type's own generated encoding id, so no id is restated here either.
  if (auto rule = DecodeGenerated<opcua::ua::IdentityMappingRuleType>(e)) {
    return scada::ExtensionObject{
        scada::ExpandedNodeId{
            scada::NodeId{scada::kIdentityMappingRuleTypeDataTypeId, 0}},
        std::any{scada::IdentityMappingRule{
            .criteria_type =
                static_cast<scada::IdentityCriteriaType>(rule->criteria_type),
            .criteria = std::string{rule->criteria}}}};
  }

  if (auto user = DecodeGenerated<opcua::ua::UserManagementDataType>(e)) {
    return scada::ExtensionObject{
        scada::ExpandedNodeId{
            scada::NodeId{scada::kUserManagementDataTypeId, 0}},
        std::any{scada::UserManagementDataType{
            .user_name = std::string{user->user_name},
            .user_configuration = static_cast<scada::UserConfiguration>(
                user->user_configuration),
            .description = std::string{user->description}}}};
  }

  if (auto range = DecodeGenerated<opcua::ua::Range>(e)) {
    return scada::ExtensionObject{
        scada::ExpandedNodeId{scada::NodeId{scada::kRangeDataTypeId, 0}},
        std::any{scada::Range{.low = range->low, .high = range->high}}};
  }

  return scada::ExtensionObject{ToScada(e.data_type_id()), {}};
}

opcua::Variant ToOpcua(const scada::Variant& v) {
  using V = scada::Variant;
  switch (v.type()) {
    case V::EMPTY:
      return {};
    case V::BOOL:
      return ToOpcuaSame<bool>(v);
    case V::INT8:
      return ToOpcuaSame<scada::Int8>(v);
    case V::UINT8:
      return ToOpcuaSame<scada::UInt8>(v);
    case V::INT16:
      return ToOpcuaSame<scada::Int16>(v);
    case V::UINT16:
      return ToOpcuaSame<scada::UInt16>(v);
    case V::INT32:
      return ToOpcuaSame<scada::Int32>(v);
    case V::UINT32:
      return ToOpcuaSame<scada::UInt32>(v);
    case V::INT64:
      return ToOpcuaSame<scada::Int64>(v);
    case V::UINT64:
      return ToOpcuaSame<scada::UInt64>(v);
    case V::DOUBLE:
      return ToOpcuaSame<scada::Double>(v);
    case V::BYTE_STRING:
      return ToOpcuaSame<scada::ByteString>(v);
    case V::STRING:
      return ToOpcuaSame<scada::String>(v);
    case V::LOCALIZED_TEXT:
      return ToOpcuaConv<scada::LocalizedText>(v);
    case V::QUALIFIED_NAME:
      return ToOpcuaConv<scada::QualifiedName>(v);
    case V::NODE_ID:
      return ToOpcuaConv<scada::NodeId>(v);
    case V::EXPANDED_NODE_ID:
      return ToOpcuaConv<scada::ExpandedNodeId>(v);
    case V::EXTENSION_OBJECT:
      return ToOpcuaConv<scada::ExtensionObject>(v);
    case V::DATE_TIME:  // no array alternative for Time
      return opcua::Variant{ToOpcua(v.get<scada::Time>())};
    default:
      return {};
  }
}

scada::Variant ToScada(const opcua::Variant& v) {
  using V = opcua::Variant;
  switch (v.type()) {
    case V::EMPTY:
      return {};
    case V::BOOL:
      return ToScadaSame<bool>(v);
    case V::INT8:
      return ToScadaSame<opcua::Int8>(v);
    case V::UINT8:
      return ToScadaSame<opcua::UInt8>(v);
    case V::INT16:
      return ToScadaSame<opcua::Int16>(v);
    case V::UINT16:
      return ToScadaSame<opcua::UInt16>(v);
    case V::INT32:
      return ToScadaSame<opcua::Int32>(v);
    case V::UINT32:
      return ToScadaSame<opcua::UInt32>(v);
    case V::INT64:
      return ToScadaSame<opcua::Int64>(v);
    case V::UINT64:
      return ToScadaSame<opcua::UInt64>(v);
    case V::DOUBLE:
      return ToScadaSame<opcua::Double>(v);
    case V::BYTE_STRING:
      return ToScadaSame<opcua::ByteString>(v);
    case V::STRING:
      return ToScadaSame<opcua::String>(v);
    case V::LOCALIZED_TEXT:
      return ToScadaConv<opcua::LocalizedText>(v);
    case V::QUALIFIED_NAME:
      return ToScadaConv<opcua::QualifiedName>(v);
    case V::NODE_ID:
      return ToScadaConv<opcua::NodeId>(v);
    case V::EXPANDED_NODE_ID:
      return ToScadaConv<opcua::ExpandedNodeId>(v);
    case V::EXTENSION_OBJECT:
      return ToScadaConv<opcua::ExtensionObject>(v);
    case V::DATE_TIME:
      return scada::Variant{ToScada(v.get<opcua::DateTime>())};
    default:
      return {};
  }
}

opcua::DataValue ToOpcua(const scada::DataValue& d) {
  opcua::DataValue out;
  out.value = ToOpcua(d.value);
  out.qualifier = ToOpcua(d.qualifier);
  out.source_timestamp = ToOpcua(d.source_timestamp);
  out.server_timestamp = ToOpcua(d.server_timestamp);
  out.status_code = ToOpcua(d.status_code);
  return out;
}
scada::DataValue ToScada(const opcua::DataValue& d) {
  scada::DataValue out;
  out.value = ToScada(d.value);
  out.qualifier = ToScada(d.qualifier);
  out.source_timestamp = ToScada(d.source_timestamp);
  out.server_timestamp = ToScada(d.server_timestamp);
  out.status_code = ToScada(d.status_code);
  return out;
}

}  // namespace scada::opcua_bridge
