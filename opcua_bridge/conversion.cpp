#include "opcua_bridge/conversion.h"

#include "opcua_bridge/vector_conversion.h"

namespace opcua_bridge {

namespace {

// Scalar-or-array conversion for an "identity" element type T (the same std
// type on both sides: bool, the numeric primitives, String, LocalizedText,
// ByteString). The opcua Variant accepts the value directly.
template <class T>
opcua::scada::Variant ToOpcuaSame(const scada::Variant& v) {
  if (v.is_array())
    return opcua::scada::Variant{v.get<std::vector<T>>()};
  return opcua::scada::Variant{v.get<T>()};
}
template <class T>
scada::Variant ToScadaSame(const opcua::scada::Variant& v) {
  if (v.is_array())
    return scada::Variant{v.get<std::vector<T>>()};
  return scada::Variant{v.get<T>()};
}

// Scalar-or-array conversion for a class element type that must be converted.
template <class T>
opcua::scada::Variant ToOpcuaConv(const scada::Variant& v) {
  if (v.is_array())
    return opcua::scada::Variant{ToOpcuaVector(v.get<std::vector<T>>())};
  return opcua::scada::Variant{ToOpcua(v.get<T>())};
}
template <class T>
scada::Variant ToScadaConv(const opcua::scada::Variant& v) {
  if (v.is_array())
    return scada::Variant{ToScadaVector(v.get<std::vector<T>>())};
  return scada::Variant{ToScada(v.get<T>())};
}

}  // namespace

opcua::scada::NodeId ToOpcua(const scada::NodeId& n) {
  switch (n.type()) {
    case scada::NodeIdType::Numeric:
      return opcua::scada::NodeId{n.numeric_id(), n.namespace_index()};
    case scada::NodeIdType::String:
      return opcua::scada::NodeId{n.string_id(), n.namespace_index()};
    case scada::NodeIdType::Opaque:
      return opcua::scada::NodeId{n.opaque_id(), n.namespace_index()};
  }
  return {};
}

scada::NodeId ToScada(const opcua::scada::NodeId& n) {
  switch (n.type()) {
    case opcua::scada::NodeIdType::Numeric:
      return scada::NodeId{n.numeric_id(), n.namespace_index()};
    case opcua::scada::NodeIdType::String:
      return scada::NodeId{n.string_id(), n.namespace_index()};
    case opcua::scada::NodeIdType::Opaque:
      return scada::NodeId{n.opaque_id(), n.namespace_index()};
  }
  return {};
}

opcua::scada::ExpandedNodeId ToOpcua(const scada::ExpandedNodeId& e) {
  return opcua::scada::ExpandedNodeId{ToOpcua(e.node_id()), e.namespace_uri(),
                                      e.server_index()};
}
scada::ExpandedNodeId ToScada(const opcua::scada::ExpandedNodeId& e) {
  return scada::ExpandedNodeId{ToScada(e.node_id()), e.namespace_uri(),
                               e.server_index()};
}

opcua::scada::QualifiedName ToOpcua(const scada::QualifiedName& q) {
  return opcua::scada::QualifiedName{q.name(), q.namespace_index()};
}
scada::QualifiedName ToScada(const opcua::scada::QualifiedName& q) {
  return scada::QualifiedName{q.name(), q.namespace_index()};
}

opcua::scada::ExtensionObject ToOpcua(const scada::ExtensionObject& e) {
  // The payload is a std::any holding scada-universe types; it cannot be
  // transferred across the type boundary by value. Boundary ExtensionObjects
  // are expected to carry only the data_type_id (the wire codec serializes the
  // body separately). Typed payloads, if ever needed, must be serialized to a
  // neutral form first.
  return opcua::scada::ExtensionObject{ToOpcua(e.data_type_id()), {}};
}
scada::ExtensionObject ToScada(const opcua::scada::ExtensionObject& e) {
  return scada::ExtensionObject{ToScada(e.data_type_id()), {}};
}

opcua::scada::Variant ToOpcua(const scada::Variant& v) {
  using V = scada::Variant;
  switch (v.type()) {
    case V::EMPTY: return {};
    case V::BOOL: return ToOpcuaSame<bool>(v);
    case V::INT8: return ToOpcuaSame<scada::Int8>(v);
    case V::UINT8: return ToOpcuaSame<scada::UInt8>(v);
    case V::INT16: return ToOpcuaSame<scada::Int16>(v);
    case V::UINT16: return ToOpcuaSame<scada::UInt16>(v);
    case V::INT32: return ToOpcuaSame<scada::Int32>(v);
    case V::UINT32: return ToOpcuaSame<scada::UInt32>(v);
    case V::INT64: return ToOpcuaSame<scada::Int64>(v);
    case V::UINT64: return ToOpcuaSame<scada::UInt64>(v);
    case V::DOUBLE: return ToOpcuaSame<scada::Double>(v);
    case V::BYTE_STRING: return ToOpcuaSame<scada::ByteString>(v);
    case V::STRING: return ToOpcuaSame<scada::String>(v);
    case V::LOCALIZED_TEXT: return ToOpcuaSame<scada::LocalizedText>(v);
    case V::QUALIFIED_NAME: return ToOpcuaConv<scada::QualifiedName>(v);
    case V::NODE_ID: return ToOpcuaConv<scada::NodeId>(v);
    case V::EXPANDED_NODE_ID: return ToOpcuaConv<scada::ExpandedNodeId>(v);
    case V::EXTENSION_OBJECT: return ToOpcuaConv<scada::ExtensionObject>(v);
    case V::DATE_TIME:  // no array alternative for DateTime
      return opcua::scada::Variant{ToOpcua(v.get<scada::DateTime>())};
    default: return {};
  }
}

scada::Variant ToScada(const opcua::scada::Variant& v) {
  using V = opcua::scada::Variant;
  switch (v.type()) {
    case V::EMPTY: return {};
    case V::BOOL: return ToScadaSame<bool>(v);
    case V::INT8: return ToScadaSame<opcua::scada::Int8>(v);
    case V::UINT8: return ToScadaSame<opcua::scada::UInt8>(v);
    case V::INT16: return ToScadaSame<opcua::scada::Int16>(v);
    case V::UINT16: return ToScadaSame<opcua::scada::UInt16>(v);
    case V::INT32: return ToScadaSame<opcua::scada::Int32>(v);
    case V::UINT32: return ToScadaSame<opcua::scada::UInt32>(v);
    case V::INT64: return ToScadaSame<opcua::scada::Int64>(v);
    case V::UINT64: return ToScadaSame<opcua::scada::UInt64>(v);
    case V::DOUBLE: return ToScadaSame<opcua::scada::Double>(v);
    case V::BYTE_STRING: return ToScadaSame<opcua::scada::ByteString>(v);
    case V::STRING: return ToScadaSame<opcua::scada::String>(v);
    case V::LOCALIZED_TEXT: return ToScadaSame<opcua::scada::LocalizedText>(v);
    case V::QUALIFIED_NAME: return ToScadaConv<opcua::scada::QualifiedName>(v);
    case V::NODE_ID: return ToScadaConv<opcua::scada::NodeId>(v);
    case V::EXPANDED_NODE_ID: return ToScadaConv<opcua::scada::ExpandedNodeId>(v);
    case V::EXTENSION_OBJECT: return ToScadaConv<opcua::scada::ExtensionObject>(v);
    case V::DATE_TIME:
      return scada::Variant{ToScada(v.get<opcua::scada::DateTime>())};
    default: return {};
  }
}

opcua::scada::DataValue ToOpcua(const scada::DataValue& d) {
  opcua::scada::DataValue out;
  out.value = ToOpcua(d.value);
  out.qualifier = ToOpcua(d.qualifier);
  out.source_timestamp = ToOpcua(d.source_timestamp);
  out.server_timestamp = ToOpcua(d.server_timestamp);
  out.status_code = ToOpcua(d.status_code);
  return out;
}
scada::DataValue ToScada(const opcua::scada::DataValue& d) {
  scada::DataValue out;
  out.value = ToScada(d.value);
  out.qualifier = ToScada(d.qualifier);
  out.source_timestamp = ToScada(d.source_timestamp);
  out.server_timestamp = ToScada(d.server_timestamp);
  out.status_code = ToScada(d.status_code);
  return out;
}

}  // namespace opcua_bridge
