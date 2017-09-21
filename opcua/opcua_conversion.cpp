#include "opcua_conversion.h"

#include "base/strings/sys_string_conversions.h"
#include "base/format_time.h"

#include <opcuapp/structs.h>

scada::NodeClass ConvertNodeClass(opcua::NodeClass node_class) {
  return static_cast<scada::NodeClass>(node_class);
}

opcua::NodeClass Convert(scada::NodeClass node_class) {
  return static_cast<opcua::NodeClass>(node_class);
}

scada::StatusCode ConvertStatusCode(opcua::StatusCode status_code) {
  if (status_code.code() == OpcUa_BadAttributeIdInvalid)
    return scada::StatusCode::Bad_WrongAttributeId;
  return status_code.IsGood() ? scada::StatusCode::Good :
         status_code.IsUncertain() ? scada::StatusCode::Uncertain :
         scada::StatusCode::Bad;
}

opcua::StatusCode MakeStatusCode(scada::StatusCode status_code) {
  if (status_code == scada::StatusCode::Bad_WrongAttributeId)
    return OpcUa_BadAttributeIdInvalid;
  return scada::IsGood(status_code) ? OpcUa_Good : OpcUa_Bad;
}

scada::String Convert(const OpcUa_String& source) {
  if (::OpcUa_String_IsNull(&source))
    return {};
  auto* raw_string = ::OpcUa_String_GetRawString(&source);
  size_t size = ::OpcUa_String_StrLen(&source);
  return scada::String{raw_string, size};
}

scada::Variant ConvertScalar(const OpcUa_Variant& source) {
  assert(source.ArrayType == OpcUa_VariantArrayType_Scalar);

  switch (source.Datatype) {
    case OpcUaType_Null:
      return {};
    case OpcUaType_Boolean:
      return source.Value.Boolean != OpcUa_False;
    case OpcUaType_Byte:
      return source.Value.Byte;
    case OpcUaType_SByte:
      return source.Value.SByte;
    case OpcUaType_Int16:
      return source.Value.Int16;
    case OpcUaType_UInt16:
      return source.Value.UInt16;
    case OpcUaType_Int32:
      return source.Value.Int32;
    case OpcUaType_UInt32:
      // TODO:
      return static_cast<int>(source.Value.UInt32);
    case OpcUaType_Int64:
      return source.Value.Int64;
    case OpcUaType_UInt64:
      // TODO:
      return static_cast<int64_t>(source.Value.UInt64);
    case OpcUaType_Float:
      return source.Value.Float;
    case OpcUaType_Double:
      return source.Value.Double;
    case OpcUaType_DateTime:
      // TODO:
      return FormatTime(Convert(source.Value.DateTime));
    case OpcUaType_String:
      return Convert(source.Value.String);
    case OpcUaType_Guid:
      // TODO:
      return {};
    case OpcUaType_ByteString:
      return Convert(source.Value.ByteString);
    case OpcUaType_XmlElement:
      // TODO:
      return {};
    case OpcUaType_NodeId:
      return Convert(*source.Value.NodeId);
    case OpcUaType_ExpandedNodeId:
      return Convert(*source.Value.ExpandedNodeId);
    case OpcUaType_StatusCode:
      // TODO:
      return {};
    case OpcUaType_QualifiedName:
      return Convert(*source.Value.QualifiedName);
    case OpcUaType_LocalizedText:
      return Convert(*source.Value.LocalizedText);
    case OpcUaType_ExtensionObject:
      // TODO:
      return {};
    case OpcUaType_DataValue:
      // TODO:
      return {};
    default:
      assert(false);
      return {};
  }
}

scada::Variant ConvertArray(OpcUa_Variant&& source) {
  assert(source.ArrayType == OpcUa_VariantArrayType_Array);
  auto& value = source.Value.Array;
  switch (source.Datatype) {
    case OpcUaType_String:
      return ConvertVector<scada::String>(value.Value.StringArray, value.Value.StringArray + value.Length);
    case OpcUaType_LocalizedText:
      return ConvertVector<scada::LocalizedText>(value.Value.LocalizedTextArray, value.Value.LocalizedTextArray + value.Length);
    case OpcUaType_ExtensionObject:
      return ConvertVector<scada::ExtensionObject>(value.Value.ExtensionObjectArray, value.Value.ExtensionObjectArray + value.Length);
    default:
      assert(false);
      return {};
  }
}

scada::Variant Convert(OpcUa_Variant&& source) {
  switch (source.ArrayType) {
    case OpcUa_VariantArrayType_Scalar:
      return ConvertScalar(source);
    case OpcUa_VariantArrayType_Array:
      return ConvertArray(std::move(source));
    default:
      // TODO:
      assert(false);
      return {};
  }
}

OpcUa_VariantArrayValue MakeVariantArrayValue(const std::vector<scada::String>& value) {
  OpcUa_VariantArrayValue result;
  result.Length = value.size();
  result.Value.StringArray = reinterpret_cast<OpcUa_String*>(::OpcUa_Memory_Alloc(sizeof(OpcUa_String) * value.size()));
  for (size_t i = 0; i < value.size(); ++i) {
    ::OpcUa_String_Initialize(&result.Value.StringArray[i]);
    ::OpcUa_String_AttachCopy(&result.Value.StringArray[i], const_cast<OpcUa_StringA>(value[i].c_str()));
  }
  return result;
}

OpcUa_VariantArrayValue MakeVariantArrayValue(std::vector<scada::ExtensionObject>&& value) {
  OpcUa_VariantArrayValue result;
  result.Length = value.size();
  result.Value.ExtensionObjectArray = reinterpret_cast<OpcUa_ExtensionObject*>(::OpcUa_Memory_Alloc(sizeof(OpcUa_ExtensionObject) * value.size()));
  for (size_t i = 0; i < value.size(); ++i) {
    ::OpcUa_ExtensionObject_Initialize(&result.Value.ExtensionObjectArray[i]);
    result.Value.ExtensionObjectArray[i] = value[i].release();
  }
  return result;
}

OpcUa_Variant MakeVariant(scada::Variant&& source) {
  static_assert(scada::Variant::COUNT == 15, "Not all types are declared");

  OpcUa_Variant result;
  OpcUa_Variant_Initialize(&result);

  if (source.is_array()) {
    assert(source.type() != scada::Variant::EMPTY);

    switch (source.type()) {
      case scada::Variant::STRING:
        result.Datatype = OpcUaType_String;
        result.ArrayType = OpcUa_VariantArrayType_Array;
        result.Value.Array = MakeVariantArrayValue(source.get<std::vector<scada::String>>());
        break;

      case scada::Variant::EXTENSION_OBJECT:
        result.Datatype = OpcUaType_ExtensionObject;
        result.ArrayType = OpcUa_VariantArrayType_Array;
        result.Value.Array = MakeVariantArrayValue(std::move(source.get<std::vector<scada::ExtensionObject>>()));
        break;

      default:
        assert(false);
        result.Datatype = OpcUaType_Null;
        break;
    }

  } else {
    switch (source.type()) {
      case scada::Variant::EMPTY:
        result.Datatype = OpcUaType_Null;
        break;

      case scada::Variant::BOOL:
        result.Datatype = OpcUaType_Boolean;
        result.Value.Boolean = source.as_bool() ? OpcUa_True : OpcUa_False;
        break;

      case scada::Variant::INT8:
        result.Datatype = OpcUaType_SByte;
        result.Value.SByte = source.get<int8_t>();
        break;

      case scada::Variant::UINT8:
        result.Datatype = OpcUaType_Byte;
        result.Value.Byte = source.get<uint8_t>();
        break;

      case scada::Variant::INT32:
        result.Datatype = OpcUaType_Int32;
        result.Value.Int32 = source.as_int32();
        break;

      case scada::Variant::UINT32:
        result.Datatype = OpcUaType_UInt32;
        result.Value.UInt32 = source.get<uint32_t>();
        break;

      case scada::Variant::INT64:
        result.Datatype = OpcUaType_Int64;
        result.Value.Int64 = source.as_int64();
        break;

      case scada::Variant::DOUBLE:
        result.Datatype = OpcUaType_Double;
        result.Value.Double = source.as_double();
        break;

      case scada::Variant::STRING:
        result.Datatype = OpcUaType_String;
        ::OpcUa_String_Initialize(&result.Value.String);
        ::OpcUa_String_AttachCopy(&result.Value.String, const_cast<OpcUa_StringA>(source.as_string().c_str()));
        break;

      case scada::Variant::QUALIFIED_NAME: {
        ::OpcUa_QualifiedName* value = static_cast<::OpcUa_QualifiedName*>(::OpcUa_Memory_Alloc(sizeof(OpcUa_QualifiedName)));
        ::OpcUa_QualifiedName_Initialize(value);
        Convert(source.get<scada::QualifiedName>(), *value);
        result.Datatype = OpcUaType_QualifiedName;
        result.Value.QualifiedName = value;
        break;
      }
      
      case scada::Variant::LOCALIZED_TEXT: {
        ::OpcUa_LocalizedText* value = static_cast<::OpcUa_LocalizedText*>(::OpcUa_Memory_Alloc(sizeof(OpcUa_LocalizedText)));
        ::OpcUa_LocalizedText_Initialize(value);
        Convert(source.get<scada::LocalizedText>(), *value);
        result.Datatype = OpcUaType_LocalizedText;
        result.Value.LocalizedText = value;
        break;
      }

      case scada::Variant::NODE_ID:
        result.Datatype = OpcUaType_NodeId;
        result.Value.NodeId = reinterpret_cast<OpcUa_NodeId*>(::OpcUa_Memory_Alloc(sizeof(OpcUa_NodeId)));
        Convert(source.as_node_id(), *result.Value.NodeId);
        break;

      case scada::Variant::EXPANDED_NODE_ID:
        result.Datatype = OpcUaType_ExpandedNodeId;
        result.Value.ExpandedNodeId = reinterpret_cast<OpcUa_ExpandedNodeId*>(::OpcUa_Memory_Alloc(sizeof(OpcUa_ExpandedNodeId)));
        Convert(source.get<scada::ExpandedNodeId>(), *result.Value.ExpandedNodeId);
        break;

      case scada::Variant::EXTENSION_OBJECT:
        result.Datatype = OpcUaType_ExtensionObject;
        ::OpcUa_ExtensionObject_Create(&result.Value.ExtensionObject);
        *result.Value.ExtensionObject = source.get<scada::ExtensionObject>().release();
        break;

      case scada::Variant::BYTE_STRING: {
        result.Datatype = OpcUaType_ByteString;
        auto& byte_string = source.get<scada::ByteString>();
        result.Value.ByteString = opcua::ByteString{byte_string.data(), byte_string.size()}.release();
        break;
      }

      default:
        assert(false);
        result.Datatype = OpcUaType_Null;
        break;
    }
  }

  return result;
}

scada::Qualifier MakeQualifier(opcua::StatusCode source) {
  scada::Qualifier result;
  result.set_bad(source.IsNotGood());
  result.set_failed(source.IsBad());
  return result;
}

scada::DateTime Convert(OpcUa_DateTime source) {
  static_assert(sizeof(source) == sizeof(int64_t), "Wrong size");
  const auto us = reinterpret_cast<const int64_t&>(source) / 10;
  if (us == 0)
    return {};

  const int64_t daysBetween1601And1970 = 134774;
  const int64_t usFrom1601To1970 = daysBetween1601And1970 * 24 * 3600 * 1000000;

  if (us < usFrom1601To1970)
    return {};

  return scada::DateTime::UnixEpoch() + base::TimeDelta::FromMicroseconds(us - usFrom1601To1970);
}

OpcUa_DateTime Convert(scada::DateTime source) {
  if (source.is_null())
    return OpcUa_DateTime{};

  const int64_t daysBetween1601And1970 = 134774;
  const int64_t usFrom1601To1970 = daysBetween1601And1970 * 24 * 3600 * 1000000;

  auto delta_us = (source - base::Time::UnixEpoch()).InMicroseconds();
  auto us = delta_us + usFrom1601To1970;
  auto ticks = us * 10;
  
  OpcUa_DateTime result;
  result.dwHighDateTime = static_cast<OpcUa_UInt32>(ticks >> 32);
  result.dwLowDateTime = static_cast<OpcUa_UInt32>(ticks);
  return result;
}

scada::DataValue Convert(OpcUa_DataValue&& source) {
  // TODO: Picoseconds.
  scada::DataValue result{
      Convert(std::move(source.Value)),
      MakeQualifier(source.StatusCode),
      Convert(source.ServerTimestamp),
      Convert(source.SourceTimestamp),
  };
  result.status_code = ConvertStatusCode(source.StatusCode);
  return result;
}

OpcUa_DataValue MakeDataValue(scada::DataValue&& source) {
  OpcUa_DataValue result;
  OpcUa_DataValue_Initialize(&result);
  result.SourceTimestamp = Convert(source.source_timestamp);
  result.ServerTimestamp = Convert(source.server_timestamp);
  result.Value = MakeVariant(std::move(source.value));
  result.StatusCode = MakeStatusCode(source.status_code).code();
  return result;
}

void Convert(const scada::NodeId& node_id, OpcUa_NodeId& result) {
  // TODO:
  result.NamespaceIndex = node_id.namespace_index();
  switch (node_id.type()) {
    case scada::NodeIdType::Numeric:
      result.IdentifierType = OpcUa_IdentifierType_Numeric;
      result.Identifier.Numeric = node_id.numeric_id();
      break;
    case scada::NodeIdType::String:
      result.IdentifierType = OpcUa_IdentifierType_String;
      OpcUa_String_AttachToString(const_cast<char*>(node_id.string_id().data()),
          node_id.string_id().size(), node_id.string_id().size(),
          OpcUa_True, OpcUa_True, &result.Identifier.String);
      break;
    default:
      assert(false);
      break;
  }
}

std::string Convert(OpcUa_Guid& guid) {
  // TODO:
  return "Guid";
}

scada::NodeId Convert(const OpcUa_NodeId& node_id) {
  switch (node_id.IdentifierType) {
    case OpcUa_IdentifierType_Numeric:
      return {node_id.Identifier.Numeric, node_id.NamespaceIndex};

    case OpcUa_IdentifierType_String:
      return {OpcUa_String_GetRawString(&node_id.Identifier.String), node_id.NamespaceIndex};

    case OpcUa_IdentifierType_Guid:
      return {Convert(*node_id.Identifier.Guid), node_id.NamespaceIndex};

    case OpcUa_IdentifierType_Opaque: {
      auto& byte_string = node_id.Identifier.ByteString;
      return {
          scada::ByteString(
              reinterpret_cast<const char*>(byte_string.Data),
              reinterpret_cast<const char*>(byte_string.Data + byte_string.Length)),
          node_id.NamespaceIndex,
      };
    }

    default:
      assert(false);
      return {};
  };
}

scada::ExpandedNodeId Convert(const OpcUa_ExpandedNodeId& node_id) {
  return {
      Convert(node_id.NodeId),
      Convert(node_id.NamespaceUri),
      node_id.ServerIndex
  };
}

void Convert(const scada::ExpandedNodeId& source, OpcUa_ExpandedNodeId& target) {
  Convert(source.node_id(), target.NodeId);
  Convert(source.namespace_uri(), target.NamespaceUri);
  target.ServerIndex = source.server_index();
}

scada::AttributeId ConvertAttributeId(OpcUa_Int32 attribute_id) {
  // TODO: Validate.
  return static_cast<scada::AttributeId>(attribute_id);
}

OpcUa_Int32 Convert(scada::AttributeId attribute_id) {
  // TODO: Validate.
  return static_cast<OpcUa_Int32>(attribute_id);
}

scada::ReadValueId Convert(const OpcUa_ReadValueId& source) {
  return {Convert(source.NodeId), ConvertAttributeId(source.AttributeId)};
}

OpcUa_ReadValueId MakeUaReadValueId(const scada::ReadValueId& source) {
  OpcUa_ReadValueId result;
  ::OpcUa_ReadValueId_Initialize(&result);
  Convert(source.first, result.NodeId);
  result.AttributeId = Convert(source.second);
  return result;
}

OpcUa_BrowseDirection Convert(scada::BrowseDirection source) {
  switch (source) {
    case scada::BrowseDirection::Forward:
      return OpcUa_BrowseDirection_Forward;
    case scada::BrowseDirection::Inverse:
      return OpcUa_BrowseDirection_Inverse;
    case scada::BrowseDirection::Both:
      return OpcUa_BrowseDirection_Both;
    default:
      assert(false);
      return OpcUa_BrowseDirection_Invalid;
  }
}

scada::BrowseDirection Convert(OpcUa_BrowseDirection source) {
  switch (source) {
    case OpcUa_BrowseDirection_Forward:
      return scada::BrowseDirection::Forward;
    case OpcUa_BrowseDirection_Inverse:
      return scada::BrowseDirection::Inverse;
    case OpcUa_BrowseDirection_Both:
      return scada::BrowseDirection::Both;
    default:
      assert(false);
      return scada::BrowseDirection::Both;
  }
}

scada::BrowseDescription Convert(const OpcUa_BrowseDescription& source) {
  return {
      Convert(source.NodeId),
      Convert(source.BrowseDirection),
      Convert(source.ReferenceTypeId),
      source.IncludeSubtypes != OpcUa_False,
  };
}

OpcUa_BrowseDescription Convert(const scada::BrowseDescription& source) {
  OpcUa_BrowseDescription result;
  OpcUa_BrowseDescription_Initialize(&result);
  Convert(source.node_id, result.NodeId);
  Convert(source.reference_type_id, result.ReferenceTypeId);
  result.IncludeSubtypes = source.include_subtypes ? OpcUa_True : OpcUa_False;
  result.BrowseDirection = Convert(source.direction);
  result.NodeClassMask = 0xFF;
  result.ResultMask = OpcUa_BrowseResultMask_ReferenceTypeInfo;
  return result;
}

scada::ReferenceDescription Convert(const OpcUa_ReferenceDescription& source) {
  return {
      Convert(source.ReferenceTypeId),
      source.IsForward != OpcUa_False,
      Convert(source.NodeId).node_id(),
  };
}

void Convert(const scada::ReferenceDescription& source, OpcUa_ReferenceDescription& result) {
  assert(!source.node_id.is_null());
  assert(!source.reference_type_id.is_null());

  Convert(source.reference_type_id, result.ReferenceTypeId);
  Convert(source.node_id, result.NodeId);
  result.IsForward = source.forward ? OpcUa_True : OpcUa_False;
}

scada::BrowseResult Convert(const OpcUa_BrowseResult& source) {
  return {
      ConvertStatusCode(source.StatusCode),
      ConvertVector<scada::ReferenceDescription>(source.References, source.References + source.NoOfReferences),
  };
}

void Convert(const scada::BrowseResult& source, OpcUa_BrowseResult& result) {
  result.StatusCode = MakeStatusCode(source.status_code).code();
  auto references = MakeVector<OpcUa_ReferenceDescription>(opcua::MakeSpan(source.references.data(), source.references.size()));
  result.NoOfReferences = references.size();
  result.References = references.release();
}

scada::ExtensionObject Convert(OpcUa_ExtensionObject&& object) {
  return std::move(object);
}

scada::String Convert(const opcua::String& source) {
  return source.raw_string();
}

void Convert(const scada::String& source, OpcUa_String& target) {
  ::OpcUa_String_AttachCopy(&target, const_cast<OpcUa_StringA>(source.c_str()));
}

scada::QualifiedName Convert(const OpcUa_QualifiedName& source) {
  return {
      Convert(source.Name),
      static_cast<scada::NamespaceIndex>(source.NamespaceIndex),
  };
}

void Convert(const scada::QualifiedName& source, OpcUa_QualifiedName& target) {
  target.NamespaceIndex = static_cast<opcua::NamespaceIndex>(source.namespace_index());
  Convert(source.name(), target.Name);
}

scada::LocalizedText Convert(const OpcUa_LocalizedText& source) {
  return Convert(source.Text);
}

void Convert(const scada::LocalizedText& source, OpcUa_LocalizedText& target) {
  ::OpcUa_String_Clear(&target.Locale);
  Convert(source.text(), target.Text);
}

scada::ByteString Convert(const OpcUa_ByteString& source) {
  if (source.Length <= 0)
    return {};
  return {reinterpret_cast<const char*>(source.Data),
          reinterpret_cast<const char*>(source.Data) + source.Length};
}
