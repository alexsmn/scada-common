#include "remote/protocol_utils.h"

#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "core/standard_node_ids.h"

scada::NodeId FromProto(const protocol::NodeId& source) {
  auto namespace_index =
      static_cast<scada::NamespaceIndex>(source.namespace_index());
  if (source.has_numeric_id())
    return scada::NodeId(source.numeric_id(), namespace_index);
  else if (source.has_string_id())
    return scada::NodeId(source.string_id(), namespace_index);
  else if (source.has_opaque_id())
    return scada::NodeId(source.opaque_id(), namespace_index);
  else
    return scada::NodeId();
}

void ToProto(const scada::NodeId& source, protocol::NodeId& target) {
  if (source.namespace_index() != 0)
    target.set_namespace_index(source.namespace_index());

  switch (source.type()) {
    case scada::NodeIdType::Numeric:
      if (source.numeric_id() != 0)
        target.set_numeric_id(source.numeric_id());
      break;
    case scada::NodeIdType::String:
      target.set_string_id(source.string_id());
      break;
    case scada::NodeIdType::Opaque:
      target.set_opaque_id(source.opaque_id().data(),
                           source.opaque_id().size());
      break;
    default:
      assert(false);
      break;
  }
}

scada::LocalizedText LocalizedTextFromProto(const std::string& source) {
  return scada::LocalizedText{base::UTF8ToUTF16(source)};
}

void LocalizedTextToProto(const scada::LocalizedText& source,
                          std::string& target) {
  target = base::UTF16ToUTF8(ToString16(source));
}

void LocalizedTextArrayToProto(
    const std::vector<scada::LocalizedText>& source,
    ::google::protobuf::RepeatedPtrField<std::string>& target) {
  assert(target.empty());
  target.Reserve(source.size());
  for (auto& s : source)
    LocalizedTextToProto(s, *target.Add());
}

std::vector<scada::LocalizedText> LocalizedTextArrayFromProto(
    const ::google::protobuf::RepeatedPtrField<std::string>& source) {
  std::vector<scada::LocalizedText> result;
  result.reserve(source.size());
  for (auto& s : source)
    result.emplace_back(LocalizedTextFromProto(s));
  return result;
}

void ByteStringToProto(const scada::ByteString& source, std::string& target) {
  target.assign(source.begin(), source.end());
}

scada::ByteString ByteStringFromProto(const std::string& source) {
  return {source.begin(), source.end()};
}

static_assert(static_cast<size_t>(scada::Variant::COUNT) == 19);

scada::Variant FromProto(const protocol::Variant& source) {
  if (source.rank() == 0) {
    switch (static_cast<scada::Variant::Type>(source.data_type())) {
      case scada::Variant::EMPTY:
        return {};
      case scada::Variant::BOOL:
        return source.bool_value();
      case scada::Variant::INT8:
        return static_cast<scada::Int8>(source.int_value());
      case scada::Variant::UINT8:
        return static_cast<scada::UInt8>(source.int_value());
      case scada::Variant::INT16:
        return static_cast<scada::Int16>(source.int_value());
      case scada::Variant::UINT16:
        return static_cast<scada::UInt16>(source.int_value());
      case scada::Variant::INT32:
        return static_cast<scada::Int32>(source.int_value());
      case scada::Variant::UINT32:
        return static_cast<scada::UInt32>(source.int_value());
      case scada::Variant::INT64:
        return static_cast<scada::Int64>(source.int_value());
      case scada::Variant::UINT64:
        return static_cast<scada::UInt64>(source.int_value());
      case scada::Variant::DOUBLE:
        return source.double_value();
      case scada::Variant::STRING:
        return scada::String{base::SysWideToNativeMB(
            base::SysUTF8ToWide(source.string_value_utf8()))};
      case scada::Variant::QUALIFIED_NAME:
        return scada::QualifiedName(base::SysWideToNativeMB(
            base::SysUTF8ToWide(source.string_value_utf8())));
      case scada::Variant::LOCALIZED_TEXT:
        return LocalizedTextFromProto(source.string_value_utf8());
      case scada::Variant::BYTE_STRING:
        return ByteStringFromProto(source.byte_string_value());
      case scada::Variant::NODE_ID:
        return FromProto(source.node_id_value());
      case scada::Variant::DATE_TIME:
        return scada::DateTime::FromDeltaSinceWindowsEpoch(
            base::TimeDelta::FromMicroseconds(source.time()));
      default:
        assert(false);
        return scada::Variant();
    }

  } else if (source.rank() == 1) {
    switch (static_cast<scada::Variant::Type>(source.data_type())) {
      case scada::Variant::LOCALIZED_TEXT:
        return LocalizedTextArrayFromProto(source.string_array_utf8());
      default:
        assert(false);
        return scada::Variant();
    }

  } else {
    assert(false);
    return scada::Variant();
  }
}

static_assert(static_cast<size_t>(scada::Variant::COUNT) == 19);

void ToProto(const scada::Variant& source, protocol::Variant& target) {
  if (source.is_null())
    return;

  target.set_data_type(static_cast<int>(source.type()));

  if (source.is_scalar()) {
    switch (source.type()) {
      case scada::Variant::EMPTY:
        break;
      case scada::Variant::BOOL:
        target.set_bool_value(source.as_bool());
        break;
      case scada::Variant::INT8:
        target.set_int_value(source.get<scada::Int8>());
        break;
      case scada::Variant::UINT8:
        target.set_int_value(source.get<scada::UInt8>());
        break;
      case scada::Variant::INT16:
        target.set_int_value(source.get<scada::Int16>());
        break;
      case scada::Variant::UINT16:
        target.set_int_value(source.get<scada::UInt16>());
        break;
      case scada::Variant::INT32:
        target.set_int_value(source.get<scada::Int32>());
        break;
      case scada::Variant::UINT32:
        target.set_int_value(source.get<scada::UInt32>());
        break;
      case scada::Variant::INT64:
        target.set_int_value(source.get<scada::Int64>());
        break;
      case scada::Variant::UINT64:
        target.set_int_value(source.get<scada::UInt64>());
        break;
      case scada::Variant::DOUBLE:
        target.set_double_value(source.get<scada::Double>());
        break;
      case scada::Variant::STRING:
        target.set_string_value_utf8(
            base::SysWideToUTF8(base::SysNativeMBToWide(source.as_string())));
        break;
      case scada::Variant::QUALIFIED_NAME:
        target.set_string_value_utf8(
            base::SysWideToUTF8(base::SysNativeMBToWide(
                source.get<scada::QualifiedName>().name())));
        break;
      case scada::Variant::LOCALIZED_TEXT:
        LocalizedTextToProto(source.as_localized_text(),
                             *target.mutable_string_value_utf8());
        break;
      case scada::Variant::BYTE_STRING:
        ByteStringToProto(source.get<scada::ByteString>(),
                          *target.mutable_byte_string_value());
        break;
      case scada::Variant::NODE_ID:
        ToProto(source.as_node_id(), *target.mutable_node_id_value());
        break;
      case scada::Variant::DATE_TIME:
        target.set_time(source.get<scada::DateTime>()
                            .ToDeltaSinceWindowsEpoch()
                            .InMicroseconds());
        break;
      default:
        assert(false);
        break;
    }

  } else {
    target.set_rank(1);
    switch (source.type()) {
      case scada::Variant::LOCALIZED_TEXT:
        LocalizedTextArrayToProto(
            source.get<std::vector<scada::LocalizedText>>(),
            *target.mutable_string_array_utf8());
        break;
      default:
        assert(false);
        break;
    }
  }
}

scada::DataValue FromProto(const protocol::DataValue& source) {
  scada::DataValue result;
  if (source.has_server_time())
    result.server_timestamp =
        base::Time::FromInternalValue(source.server_time());
  if (source.has_source_time())
    result.source_timestamp =
        base::Time::FromInternalValue(source.source_time());
  if (source.has_value())
    result.value = FromProto(source.value());
  result.qualifier = scada::Qualifier(source.qualifier());
  result.status_code =
      source.has_status_code()
          ? static_cast<scada::StatusCode>(source.status_code())
          : scada::StatusCode::Good;
  return result;
}

void ToProto(const scada::DataValue& source, protocol::DataValue& target) {
  if (!source.server_timestamp.is_null())
    target.set_server_time(source.server_timestamp.ToInternalValue());
  if (!source.source_timestamp.is_null())
    target.set_source_time(source.source_timestamp.ToInternalValue());
  if (!source.value.is_null())
    ToProto(source.value, *target.mutable_value());
  target.set_qualifier(source.qualifier.raw());
  if (source.status_code != scada::StatusCode::Good)
    target.set_status_code(
        static_cast<google::protobuf::uint32>(source.status_code));
}

scada::Status FromProto(const protocol::Status& source) {
  return scada::Status::FromFullCode(source.code());
}

void ToProto(const scada::Status& source, protocol::Status& target) {
  target.set_code(source.full_code());
}

scada::Event FromProto(const protocol::Event& source) {
  scada::Event result;
  result.time = base::Time::FromInternalValue(source.time());
  result.severity = source.severity();
  if (source.has_source_node_id())
    result.node_id = FromProto(source.source_node_id());
  if (source.has_user_node_id())
    result.user_id = FromProto(source.user_node_id());
  if (source.has_value())
    result.value = FromProto(source.value());
  if (source.has_qualifier())
    result.qualifier = scada::Qualifier(source.qualifier());
  if (source.has_message_utf8())
    result.message = base::UTF8ToUTF16(source.message_utf8());
  if (source.has_acknowledged())
    result.acked = source.acknowledged();
  if (source.has_acknowledge_id())
    result.acknowledge_id = source.acknowledge_id();
  if (source.has_acknowledge_time())
    result.acknowledged_time =
        base::Time::FromInternalValue(source.acknowledge_time());
  if (source.has_acknowledge_user_id())
    result.acknowledged_user_id = FromProto(source.acknowledge_user_id());
  return result;
}

void ToProto(const scada::Event& source, protocol::Event& target) {
  target.set_time(source.time.ToInternalValue());
  target.set_severity(source.severity);
  if (!source.node_id.is_null())
    ToProto(source.node_id, *target.mutable_source_node_id());
  if (!source.user_id.is_null())
    ToProto(source.user_id, *target.mutable_user_node_id());
  if (!source.value.is_null())
    ToProto(source.value, *target.mutable_value());
  if (source.qualifier != scada::Qualifier())
    target.set_qualifier(source.qualifier.raw());
  if (!source.message.empty())
    target.set_message_utf8(base::UTF16ToUTF8(source.message));
  if (source.acked)
    target.set_acknowledged(true);
  if (source.acknowledge_id)
    target.set_acknowledge_id(source.acknowledge_id);
  if (!source.acknowledged_time.is_null())
    target.set_acknowledge_time(source.acknowledged_time.ToInternalValue());
  if (!source.acknowledged_user_id.is_null())
    ToProto(source.acknowledged_user_id, *target.mutable_acknowledge_user_id());
  target.set_acknowledge_id(source.acknowledge_id);
}

scada::NodeClass FromProto(const protocol::NodeClass source) {
  return static_cast<scada::NodeClass>(static_cast<int>(source));
}

protocol::NodeClass ToProto(const scada::NodeClass source) {
  return static_cast<protocol::NodeClass>(static_cast<int>(source));
}

scada::NodeAttributes FromProto(const protocol::Attributes& source) {
  scada::NodeAttributes result;
  if (source.has_browse_name_utf8())
    result.browse_name = scada::QualifiedName{source.browse_name_utf8(), 0};
  if (source.has_display_name_utf8())
    result.display_name =
        scada::ToLocalizedText(base::UTF8ToUTF16(source.display_name_utf8()));
  if (source.has_data_type_id())
    result.data_type = FromProto(source.data_type_id());
  if (source.has_value())
    result.value = FromProto(source.value());
  return result;
}

void ToProto(const scada::NodeAttributes& source,
             protocol::Attributes& target) {
  if (!source.browse_name.empty())
    target.set_browse_name_utf8(source.browse_name.name());
  if (!source.display_name.empty())
    target.set_display_name_utf8(
        base::UTF16ToUTF8(ToString16(source.display_name)));
  if (!source.data_type.is_null())
    ToProto(source.data_type, *target.mutable_data_type_id());
  if (!source.value.is_null())
    ToProto(source.value, *target.mutable_value());
}

scada::AttributeId FromProto(protocol::AttributeId source) {
  return static_cast<scada::AttributeId>(source);
}

protocol::AttributeId ToProto(scada::AttributeId source) {
  return static_cast<protocol::AttributeId>(source);
}

scada::ReadValueId FromProto(const protocol::ReadValueId& source) {
  return {FromProto(source.node_id()), FromProto(source.attribute_id())};
}

void ToProto(const scada::ReadValueId& source, protocol::ReadValueId& target) {
  ToProto(source.node_id, *target.mutable_node_id());
  target.set_attribute_id(ToProto(source.attribute_id));
}

scada::BrowseDirection FromProto(protocol::BrowseDirection source) {
  return static_cast<scada::BrowseDirection>(source);
}

protocol::BrowseDirection ToProto(scada::BrowseDirection source) {
  return static_cast<protocol::BrowseDirection>(source);
}

scada::ReferenceDescription FromProto(
    const protocol::ReferenceDescription& source) {
  return {
      FromProto(source.reference_type_id()),
      source.forward(),
      FromProto(source.node_id()),
  };
}

void ToProto(const scada::ReferenceDescription& source,
             protocol::ReferenceDescription& target) {
  ToProto(source.reference_type_id, *target.mutable_reference_type_id());
  target.set_forward(source.forward);
  ToProto(source.node_id, *target.mutable_node_id());
}

scada::BrowseResult FromProto(const protocol::BrowseResult& source) {
  return {
      static_cast<scada::StatusCode>(source.status_code()),
      VectorFromProto<scada::ReferenceDescription>(source.references()),
  };
}

void ToProto(const scada::BrowseResult& source,
             protocol::BrowseResult& target) {
  if (source.status_code != scada::StatusCode::Good)
    target.set_status_code(static_cast<uint32_t>(source.status_code));
  ContainerToProto(source.references, *target.mutable_references());
}

bool AssertValid(const scada::ModelChangeEvent& event) {
  assert(!event.node_id.is_null());
  assert(event.verb != 0);

  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    assert(event.verb ==
           static_cast<uint8_t>(scada::ModelChangeEvent::NodeDeleted));
  } else {
    // assert(!event.type_definition_id.is_null());
  }

  return true;
}

scada::ModelChangeEvent FromProto(const protocol::ModelChangeEvent& source) {
  scada::ModelChangeEvent result{};

  if (source.has_node_id())
    result.node_id = FromProto(source.node_id());
  if (source.has_type_definition_id())
    result.type_definition_id = FromProto(source.type_definition_id());

  if (source.node_added())
    result.verb |= scada::ModelChangeEvent::NodeAdded;
  if (source.node_deleted())
    result.verb |= scada::ModelChangeEvent::NodeDeleted;
  if (source.reference_added())
    result.verb |= scada::ModelChangeEvent::ReferenceAdded;
  if (source.reference_deleted())
    result.verb |= scada::ModelChangeEvent::ReferenceDeleted;

  assert(AssertValid(result));
  return result;
}

void ToProto(const scada::ModelChangeEvent& source,
             protocol::ModelChangeEvent& target) {
  assert(AssertValid(source));

  if (!source.node_id.is_null())
    ToProto(source.node_id, *target.mutable_node_id());
  if (!source.type_definition_id.is_null())
    ToProto(source.type_definition_id, *target.mutable_type_definition_id());

  if (source.verb & scada::ModelChangeEvent::NodeAdded)
    target.set_node_added(true);
  if (source.verb & scada::ModelChangeEvent::NodeDeleted)
    target.set_node_deleted(true);
  if (source.verb & scada::ModelChangeEvent::ReferenceAdded)
    target.set_reference_added(true);
  if (source.verb & scada::ModelChangeEvent::ReferenceDeleted)
    target.set_reference_deleted(true);
}

scada::Aggregation FromProto(const protocol::Aggregation& source) {
  return {
      scada::Duration::FromInternalValue(source.interval()),
      FromProto(source.aggregation_id()),
  };
}

void ToProto(const scada::Aggregation& source, protocol::Aggregation& target) {
  ToProto(source.aggregation_id, *target.mutable_aggregation_id());
  target.set_interval(source.interval.ToInternalValue());
}
