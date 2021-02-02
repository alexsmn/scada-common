#include "opcua_conversion.h"

#include "base/bit_cast.h"
#include "base/format_time.h"
#include "base/strings/sys_string_conversions.h"

#include <opcuapp/structs.h>

scada::NodeClass ConvertNodeClass(opcua::NodeClass node_class) {
  return static_cast<scada::NodeClass>(node_class);
}

opcua::NodeClass Convert(scada::NodeClass node_class) {
  return static_cast<opcua::NodeClass>(node_class);
}

constexpr std::pair<OpcUa_StatusCode, scada::StatusCode> kStatusCodeMapping[] =
    {
        {OpcUa_Good, scada::StatusCode::Good},
        {OpcUa_Bad, scada::StatusCode::Bad},
        {OpcUa_Uncertain, scada::StatusCode::Uncertain},
        {OpcUa_BadAttributeIdInvalid, scada::StatusCode::Bad_WrongAttributeId},
        {OpcUa_BadNodeIdInvalid, scada::StatusCode::Bad_WrongNodeId},
        {OpcUa_BadNodeClassInvalid, scada::StatusCode::Bad_WrongNodeClass},
};

scada::StatusCode ConvertStatusCode(OpcUa_StatusCode status_code) {
  auto i =
      std::find_if(std::begin(kStatusCodeMapping), std::end(kStatusCodeMapping),
                   [status_code](auto& p) { return p.first == status_code; });
  if (i != std::end(kStatusCodeMapping))
    return i->second;

  return OpcUa_IsGood(status_code)
             ? scada::StatusCode::Good
             : OpcUa_IsUncertain(status_code) ? scada::StatusCode::Uncertain
                                              : scada::StatusCode::Bad;
}

opcua::StatusCode MakeStatusCode(scada::StatusCode status_code) {
  auto i =
      std::find_if(std::begin(kStatusCodeMapping), std::end(kStatusCodeMapping),
                   [status_code](auto& p) { return p.second == status_code; });
  if (i != std::end(kStatusCodeMapping))
    return i->first;

  return scada::IsGood(status_code) ? OpcUa_Good : OpcUa_Bad;
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
      return source.Value.UInt32;
    case OpcUaType_Int64:
      return static_cast<scada::Int64>(source.Value.Int64);
    case OpcUaType_UInt64:
      return static_cast<scada::UInt64>(source.Value.UInt64);
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
    case OpcUaType_Int32:
      return std::vector<scada::Int32>(value.Value.Int32Array,
                                       value.Value.Int32Array + value.Length);
    case OpcUaType_UInt32:
      return std::vector<scada::UInt32>(value.Value.UInt32Array,
                                        value.Value.UInt32Array + value.Length);
    case OpcUaType_String:
      return ConvertVector<scada::String>(
          value.Value.StringArray, value.Value.StringArray + value.Length);
    case OpcUaType_LocalizedText:
      return ConvertVector<scada::LocalizedText>(
          value.Value.LocalizedTextArray,
          value.Value.LocalizedTextArray + value.Length);
    case OpcUaType_ExtensionObject:
      return ConvertVector<scada::ExtensionObject>(
          value.Value.ExtensionObjectArray,
          value.Value.ExtensionObjectArray + value.Length);
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

void MicrosecondsToDateTime(int64_t us, OpcUa_DateTime* ft) {
  DCHECK_GE(us, 0LL) << "Time is less than 0, negative values are not "
                        "representable in FILETIME";

  // Multiply by 10 to convert microseconds to 100-nanoseconds. Bit_cast will
  // handle alignment problems. This only works on little-endian machines.
  *ft = bit_cast<OpcUa_DateTime, int64_t>(us * 10);
}

OpcUa_DateTime Convert(scada::DateTime time) {
  if (time.is_null())
    return bit_cast<OpcUa_DateTime, int64_t>(0);
  if (time.is_max()) {
    OpcUa_DateTime result;
    result.dwHighDateTime = std::numeric_limits<OpcUa_UInt32>::max();
    result.dwLowDateTime = std::numeric_limits<OpcUa_UInt32>::max();
    return result;
  }
  OpcUa_DateTime utc_ft;
  MicrosecondsToDateTime(time.ToDeltaSinceWindowsEpoch().InMicroseconds(),
                         &utc_ft);
  return utc_ft;
}

OpcUa_VariantArrayValue MakeVariantArrayValue(
    const std::vector<scada::String>& value) {
  auto vector =
      MakeVector<OpcUa_String>(opcua::MakeSpan(value.data(), value.size()));
  OpcUa_VariantArrayValue result;
  result.Length = value.size();
  result.Value.StringArray = vector.release();
  return result;
}

OpcUa_VariantArrayValue MakeVariantArrayValue(
    const std::vector<scada::LocalizedText>& value) {
  auto vector = MakeVector<OpcUa_LocalizedText>(
      opcua::MakeSpan(value.data(), value.size()));
  OpcUa_VariantArrayValue result;
  result.Length = value.size();
  result.Value.LocalizedTextArray = vector.release();
  return result;
}

OpcUa_VariantArrayValue MakeVariantArrayValue(
    std::vector<scada::ExtensionObject>&& value) {
  auto vector = MakeVector<OpcUa_ExtensionObject>(
      opcua::MakeSpan(value.data(), value.size()));
  OpcUa_VariantArrayValue result;
  result.Length = value.size();
  result.Value.ExtensionObjectArray = vector.release();
  return result;
}

void Convert(scada::Variant&& source, OpcUa_Variant& result) {
  static_assert(scada::Variant::COUNT == 19, "Not all types are declared");

  if (source.is_array()) {
    assert(source.type() != scada::Variant::EMPTY);

    switch (source.type()) {
      case scada::Variant::STRING:
        result.Datatype = OpcUaType_String;
        result.ArrayType = OpcUa_VariantArrayType_Array;
        result.Value.Array =
            MakeVariantArrayValue(source.get<std::vector<scada::String>>());
        break;

      case scada::Variant::LOCALIZED_TEXT:
        result.Datatype = OpcUaType_LocalizedText;
        result.ArrayType = OpcUa_VariantArrayType_Array;
        result.Value.Array = MakeVariantArrayValue(
            source.get<std::vector<scada::LocalizedText>>());
        break;

      case scada::Variant::EXTENSION_OBJECT:
        result.Datatype = OpcUaType_ExtensionObject;
        result.ArrayType = OpcUa_VariantArrayType_Array;
        result.Value.Array = MakeVariantArrayValue(
            std::move(source.get<std::vector<scada::ExtensionObject>>()));
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
        result.Value.SByte = source.get<scada::Int8>();
        break;

      case scada::Variant::UINT8:
        result.Datatype = OpcUaType_Byte;
        result.Value.Byte = source.get<scada::UInt8>();
        break;

      case scada::Variant::INT16:
        result.Datatype = OpcUaType_Int16;
        result.Value.Int16 = source.get<scada::Int16>();
        break;

      case scada::Variant::UINT16:
        result.Datatype = OpcUaType_UInt16;
        result.Value.UInt16 = source.get<scada::UInt16>();
        break;

      case scada::Variant::INT32:
        result.Datatype = OpcUaType_Int32;
        result.Value.Int32 = source.as_int32();
        break;

      case scada::Variant::UINT32:
        result.Datatype = OpcUaType_UInt32;
        result.Value.UInt32 = source.get<scada::UInt32>();
        break;

      case scada::Variant::INT64:
        result.Datatype = OpcUaType_Int64;
        result.Value.Int64 = source.as_int64();
        break;

      case scada::Variant::UINT64:
        result.Datatype = OpcUaType_UInt64;
        result.Value.Int64 = source.as_uint64();
        break;

      case scada::Variant::DOUBLE:
        result.Datatype = OpcUaType_Double;
        result.Value.Double = source.as_double();
        break;

      case scada::Variant::STRING:
        result.Datatype = OpcUaType_String;
        ::OpcUa_String_Initialize(&result.Value.String);
        ::OpcUa_String_AttachCopy(
            &result.Value.String,
            const_cast<OpcUa_StringA>(source.as_string().c_str()));
        break;

      case scada::Variant::QUALIFIED_NAME: {
        ::OpcUa_QualifiedName* value = static_cast<::OpcUa_QualifiedName*>(
            ::OpcUa_Memory_Alloc(sizeof(OpcUa_QualifiedName)));
        ::OpcUa_QualifiedName_Initialize(value);
        Convert(source.get<scada::QualifiedName>(), *value);
        result.Datatype = OpcUaType_QualifiedName;
        result.Value.QualifiedName = value;
        break;
      }

      case scada::Variant::LOCALIZED_TEXT: {
        ::OpcUa_LocalizedText* value = static_cast<::OpcUa_LocalizedText*>(
            ::OpcUa_Memory_Alloc(sizeof(OpcUa_LocalizedText)));
        ::OpcUa_LocalizedText_Initialize(value);
        Convert(source.get<scada::LocalizedText>(), *value);
        result.Datatype = OpcUaType_LocalizedText;
        result.Value.LocalizedText = value;
        break;
      }

      case scada::Variant::NODE_ID:
        result.Datatype = OpcUaType_NodeId;
        result.Value.NodeId = reinterpret_cast<OpcUa_NodeId*>(
            ::OpcUa_Memory_Alloc(sizeof(OpcUa_NodeId)));
        Convert(source.as_node_id(), *result.Value.NodeId);
        break;

      case scada::Variant::EXPANDED_NODE_ID:
        result.Datatype = OpcUaType_ExpandedNodeId;
        result.Value.ExpandedNodeId = reinterpret_cast<OpcUa_ExpandedNodeId*>(
            ::OpcUa_Memory_Alloc(sizeof(OpcUa_ExpandedNodeId)));
        Convert(source.get<scada::ExpandedNodeId>(),
                *result.Value.ExpandedNodeId);
        break;

      case scada::Variant::EXTENSION_OBJECT: {
        result.Datatype = OpcUaType_ExtensionObject;
        ::OpcUa_ExtensionObject_Create(&result.Value.ExtensionObject);
        Convert(std::move(source.get<scada::ExtensionObject>()),
                *result.Value.ExtensionObject);
        break;
      }

      case scada::Variant::BYTE_STRING: {
        result.Datatype = OpcUaType_ByteString;
        auto& byte_string = source.get<scada::ByteString>();
        result.Value.ByteString =
            opcua::ByteString{byte_string.data(), byte_string.size()}.release();
        break;
      }

      case scada::Variant::DATE_TIME: {
        result.Datatype = OpcUaType_DateTime;
        result.Value.DateTime = Convert(source.get<scada::DateTime>());
        break;
      }

      default:
        assert(false);
        result.Datatype = OpcUaType_Null;
        break;
    }
  }
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

  return scada::DateTime::UnixEpoch() +
         base::TimeDelta::FromMicroseconds(us - usFrom1601To1970);
}

/*OpcUa_DateTime Convert(scada::DateTime source) {
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
}*/

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

void Convert(scada::DataValue&& source, OpcUa_DataValue& result) {
  result.SourceTimestamp = Convert(source.source_timestamp);
  result.ServerTimestamp = Convert(source.server_timestamp);
  Convert(std::move(source.value), result.Value);
  result.StatusCode = MakeStatusCode(source.status_code).code();
}

void Convert(const scada::NodeId& source, OpcUa_NodeId& result) {
  result.NamespaceIndex = source.namespace_index();

  switch (source.type()) {
    case scada::NodeIdType::Numeric:
      result.IdentifierType = OpcUa_IdentifierType_Numeric;
      result.Identifier.Numeric = source.numeric_id();
      break;

    case scada::NodeIdType::String:
      result.IdentifierType = OpcUa_IdentifierType_String;
      Convert(source.string_id(), result.Identifier.String);
      break;

    case scada::NodeIdType::Opaque:
      result.IdentifierType = OpcUa_IdentifierType_Opaque;
      Convert(source.opaque_id(), result.Identifier.ByteString);
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
      return {Convert(node_id.Identifier.String), node_id.NamespaceIndex};

    case OpcUa_IdentifierType_Guid:
      return {Convert(*node_id.Identifier.Guid), node_id.NamespaceIndex};

    case OpcUa_IdentifierType_Opaque:
      return {Convert(node_id.Identifier.ByteString), node_id.NamespaceIndex};

    default:
      assert(false);
      return {};
  };
}

scada::NodeId Convert(OpcUa_NodeId&& node_id) {
  switch (node_id.IdentifierType) {
    case OpcUa_IdentifierType_Numeric:
      return {node_id.Identifier.Numeric, node_id.NamespaceIndex};

    case OpcUa_IdentifierType_String:
      return {Convert(std::move(node_id.Identifier.String)),
              node_id.NamespaceIndex};

      //    case OpcUa_IdentifierType_Guid:
      //      return {Convert(std::move(*node_id.Identifier.Guid)),
      //      node_id.NamespaceIndex};

    case OpcUa_IdentifierType_Opaque:
      return {Convert(std::move(node_id.Identifier.ByteString)),
              node_id.NamespaceIndex};

    default:
      assert(false);
      return {};
  };
}

scada::ExpandedNodeId Convert(const OpcUa_ExpandedNodeId& node_id) {
  return {Convert(node_id.NodeId), Convert(node_id.NamespaceUri),
          node_id.ServerIndex};
}

void Convert(const scada::ExpandedNodeId& source,
             OpcUa_ExpandedNodeId& target) {
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

scada::ReadValueId Convert(OpcUa_ReadValueId&& source) {
  return {Convert(std::move(source.NodeId)),
          ConvertAttributeId(source.AttributeId)};
}

void Convert(const scada::ReadValueId& source, OpcUa_ReadValueId& target) {
  ::OpcUa_ReadValueId_Clear(&target);
  Convert(source.node_id, target.NodeId);
  target.AttributeId = Convert(source.attribute_id);
}

scada::AggregateFilter Convert(OpcUa_AggregateFilter&& source) {
  return {
      Convert(source.StartTime),
      scada::Duration::FromSecondsD(source.ProcessingInterval),
      Convert(std::move(source.AggregateType)),
  };
}

void Convert(const scada::AggregateFilter& source,
             OpcUa_AggregateFilter& target) {
  target.StartTime = Convert(source.start_time);
  target.ProcessingInterval = source.interval.InSecondsF();
  Convert(source.aggregate_type, target.AggregateType);
}

scada::MonitoringParameters Convert(OpcUa_MonitoringParameters&& source) {
  scada::MonitoringParameters target;
  opcua::ExtensionObject filter{std::move(source.Filter)};
  if (auto* aggregate_filter = filter.get_if<OpcUa_AggregateFilter>())
    target.filter = Convert(std::move(*aggregate_filter));
  return target;
}

void Convert(const scada::MonitoringParameters& source,
             OpcUa_MonitoringParameters& target) {
  if (auto* aggregate_filter =
          std::get_if<scada::AggregateFilter>(&source.filter)) {
    auto target_aggregate_filter = std::make_unique<OpcUa_AggregateFilter>();
    opcua::Initialize(*target_aggregate_filter);
    Convert(*aggregate_filter, *target_aggregate_filter);
    opcua::ExtensionObject target_filter{target_aggregate_filter.release()};
    target_filter.release(target.Filter);
  }
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

void Convert(const scada::ReferenceDescription& source,
             OpcUa_ReferenceDescription& result) {
  assert(!source.node_id.is_null());
  assert(!source.reference_type_id.is_null());

  Convert(source.reference_type_id, result.ReferenceTypeId);
  Convert(source.node_id, result.NodeId);
  result.IsForward = source.forward ? OpcUa_True : OpcUa_False;
}

scada::BrowseResult Convert(const OpcUa_BrowseResult& source) {
  return {
      ConvertStatusCode(source.StatusCode),
      ConvertVector<scada::ReferenceDescription>(
          source.References, source.References + source.NoOfReferences),
  };
}

void Convert(const scada::BrowseResult& source, OpcUa_BrowseResult& result) {
  result.StatusCode = MakeStatusCode(source.status_code).code();
  auto references = MakeVector<OpcUa_ReferenceDescription>(
      opcua::MakeSpan(source.references.data(), source.references.size()));
  result.NoOfReferences = references.size();
  result.References = references.release();
}

scada::ExtensionObject Convert(OpcUa_ExtensionObject&& object) {
  if (object.Encoding == OpcUa_ExtensionObjectEncoding_None)
    return {};

  auto type_id = Convert(object.TypeId);
  opcua::ExtensionObject ext{std::move(object)};
  return scada::ExtensionObject{std::move(type_id), std::move(ext)};
}

void Convert(scada::ExtensionObject&& source, OpcUa_ExtensionObject& target) {
  opcua::ExtensionObject* extension_object =
      std::any_cast<opcua::ExtensionObject>(&source.value());
  if (extension_object)
    extension_object->release(target);
}

scada::String Convert(const OpcUa_String& source) {
  if (::OpcUa_String_IsNull(&source))
    return {};
  auto* raw_string = ::OpcUa_String_GetRawString(&source);
  size_t size = ::OpcUa_String_StrSize(&source);
  return scada::String{raw_string, size};
}

scada::String Convert(OpcUa_String&& source) {
  return Convert(static_cast<const OpcUa_String&>(source));
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
  target.NamespaceIndex =
      static_cast<opcua::NamespaceIndex>(source.namespace_index());
  Convert(source.name(), target.Name);
}

scada::LocalizedText Convert(const OpcUa_LocalizedText& source) {
  return base::SysUTF8ToWide(Convert(source.Text));
}

void Convert(const scada::LocalizedText& source, OpcUa_LocalizedText& target) {
  ::OpcUa_String_Clear(&target.Locale);
  Convert(base::SysWideToUTF8(source), target.Text);
}

scada::ByteString Convert(const OpcUa_ByteString& source) {
  if (source.Length <= 0)
    return {};
  return {reinterpret_cast<const char*>(source.Data),
          reinterpret_cast<const char*>(source.Data) + source.Length};
}

void Convert(const scada::ByteString& source, OpcUa_ByteString& target) {
  OpcUa_Byte* data =
      static_cast<OpcUa_Byte*>(::OpcUa_Memory_Alloc(source.size()));
  memcpy(data, source.data(), source.size());
  target.Length = static_cast<OpcUa_UInt32>(source.size());
  target.Data = data;
}

scada::AddNodesItem Convert(const OpcUa_AddNodesItem& source) {
  return {
      Convert(source.RequestedNewNodeId.NodeId),
      Convert(source.ParentNodeId.NodeId),
      ConvertNodeClass(source.NodeClass),
      Convert(source.TypeDefinition.NodeId),
  };
}

void Convert(const scada::AddNodesResult& source,
             OpcUa_AddNodesResult& target) {
  target.StatusCode = MakeStatusCode(source.status_code).code();
  Convert(source.added_node_id, target.AddedNodeId);
}

scada::DeleteNodesItem Convert(const OpcUa_DeleteNodesItem& source) {
  return {Convert(source.NodeId), source.DeleteTargetReferences != OpcUa_False};
}

scada::WriteValueId Convert(OpcUa_WriteValue&& source) {
  return {
      Convert(source.NodeId),
      static_cast<scada::AttributeId>(source.AttributeId),
      Convert(std::move(source.Value.Value)),
      {},
  };
}

void Convert(scada::WriteValueId&& source, OpcUa_WriteValue& target) {
  target.AttributeId = static_cast<OpcUa_UInt32>(source.attribute_id);
  Convert(std::move(source.node_id), target.NodeId);
  auto now = scada::DateTime::Now();
  Convert(scada::DataValue{std::move(source.value), {}, now, now},
          target.Value);
}

scada::RelativePathElement Convert(OpcUa_RelativePathElement&& source) {
  return {
      Convert(std::move(source.ReferenceTypeId)),
      source.IsInverse != OpcUa_False,
      source.IncludeSubtypes != OpcUa_False,
      Convert(std::move(source.TargetName)),
  };
}

scada::BrowsePath Convert(OpcUa_BrowsePath&& source) {
  return {
      Convert(std::move(source.StartingNode)),
      ConvertVector<scada::RelativePathElement>(opcua::MakeSpan(
          source.RelativePath.Elements, source.RelativePath.NoOfElements)),
  };
}

void Convert(scada::BrowsePathTarget&& source, OpcUa_BrowsePathTarget& target) {
  target.RemainingPathIndex = source.remaining_path_index;
  Convert(std::move(source.target_id), target.TargetId);
}

void Convert(scada::BrowsePathResult&& source, OpcUa_BrowsePathResult& target) {
  target.StatusCode = MakeStatusCode(source.status_code).code();
  auto opcua_targets =
      ConvertFromVector<OpcUa_BrowsePathTarget>(std::move(source.targets));
  target.NoOfTargets = opcua_targets.size();
  target.Targets = opcua_targets.release();
}
