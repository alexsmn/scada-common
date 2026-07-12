#include "opcua_bridge/conversion.h"

#include "opcua_bridge/vector_conversion.h"
#include "scada/authorization.h"
#include "scada/identity_mapping_rule_encoding.h"

#include "opcua/transport/binary/codec_utils.h"

#include <any>
#include <cstdint>
#include <vector>

namespace scada::opcua_bridge {

namespace {

// Scalar-or-array conversion for an "identity" element type T (the same std
// type on both sides: bool, the numeric primitives, String, LocalizedText,
// ByteString). The opcua Variant accepts the value directly.
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
  // RolePermissionType payload: serialize it to OPC UA binary and carry the
  // bytes as the ExtensionObject body, tagged with the DefaultBinary encoding
  // NodeId (ns=0;i=128). The wire codec (AppendExtensionObjectValue) writes a
  // std::vector<char> body verbatim. OPC UA Part 3 §8.56 RolePermissionType,
  // Part 6 §5.1.8 ExtensionObject.
  if (const auto* role = std::any_cast<scada::RolePermissionType>(&e.value())) {
    std::vector<char> body;
    opcua::binary::Encoder encoder{body};
    encoder.Encode(ToOpcua(role->role_id));
    encoder.Encode(static_cast<std::uint32_t>(role->permissions));
    // RolePermissionType_Encoding_DefaultBinary, namespace 0.
    return opcua::ExtensionObject{
        opcua::ExpandedNodeId{opcua::NodeId{/*numeric_id=*/128, 0}},
        std::any{std::move(body)}};
  }

  // IdentityMappingRuleType payload (a Role's Identities entry or an
  // AddIdentity/RemoveIdentity argument): CriteriaType (Int32 enum) followed
  // by Criteria (String), per the official 1.05 Opc.Ua.Types.bsd.xml; tagged
  // with IdentityMappingRuleType_Encoding_DefaultBinary (ns=0;i=15736).
  // OPC UA Part 18 §4.4.3,
  // https://reference.opcfoundation.org/Core/Part18/v105/docs/4.4.3
  if (const auto* rule =
          std::any_cast<scada::IdentityMappingRule>(&e.value())) {
    std::vector<char> body;
    opcua::binary::Encoder encoder{body};
    encoder.Encode(static_cast<std::int32_t>(rule->criteria_type));
    encoder.Encode(std::string_view{rule->criteria});
    return opcua::ExtensionObject{
        opcua::ExpandedNodeId{
            opcua::NodeId{scada::kIdentityMappingRuleTypeDefaultBinaryId, 0}},
        std::any{std::move(body)}};
  }

  // Other payloads cannot be transferred across the type boundary by value; the
  // boundary ExtensionObject then carries only its data_type_id.
  return opcua::ExtensionObject{ToOpcua(e.data_type_id()), {}};
}
scada::ExtensionObject ToScada(const opcua::ExtensionObject& e) {
  // IdentityMappingRuleType bodies are decoded back into the typed rule so
  // method implementations (AddIdentity/RemoveIdentity, OPC UA Part 18
  // §4.4.5/§4.4.6) receive a structured argument. The wire decoder
  // (ReadExtensionObjectValue) delivers the body as an opcua::ByteString
  // (std::vector<char>); the in-process encode path above uses the same type.
  if (e.data_type_id().node_id() ==
      opcua::NodeId{scada::kIdentityMappingRuleTypeDefaultBinaryId, 0}) {
    if (const auto* body = std::any_cast<opcua::ByteString>(&e.value())) {
      opcua::binary::Decoder decoder{*body};
      std::int32_t criteria_type = 0;
      opcua::String criteria;
      if (decoder.Decode(criteria_type) && decoder.Decode(criteria)) {
        return scada::ExtensionObject{
            scada::ExpandedNodeId{
                scada::NodeId{scada::kIdentityMappingRuleTypeDataTypeId, 0}},
            std::any{scada::IdentityMappingRule{
                .criteria_type =
                    static_cast<scada::IdentityCriteriaType>(criteria_type),
                .criteria = std::string{criteria}}}};
      }
    }
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
      return ToOpcuaSame<scada::LocalizedText>(v);
    case V::QUALIFIED_NAME:
      return ToOpcuaConv<scada::QualifiedName>(v);
    case V::NODE_ID:
      return ToOpcuaConv<scada::NodeId>(v);
    case V::EXPANDED_NODE_ID:
      return ToOpcuaConv<scada::ExpandedNodeId>(v);
    case V::EXTENSION_OBJECT:
      return ToOpcuaConv<scada::ExtensionObject>(v);
    case V::DATE_TIME:  // no array alternative for DateTime
      return opcua::Variant{ToOpcua(v.get<scada::DateTime>())};
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
      return ToScadaSame<opcua::LocalizedText>(v);
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
