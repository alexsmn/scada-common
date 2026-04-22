#include "opcua/opcua_binary_service_codec.h"

#include "opcua/opcua_binary_codec_utils.h"

#include "scada/localized_text.h"

namespace opcua {
namespace {

constexpr std::uint32_t kCreateSessionRequestBinaryEncodingId = 461;
constexpr std::uint32_t kActivateSessionRequestBinaryEncodingId = 467;
constexpr std::uint32_t kCloseSessionRequestBinaryEncodingId = 473;
constexpr std::uint32_t kBrowseRequestBinaryEncodingId = 525;
constexpr std::uint32_t kReadRequestBinaryEncodingId = 629;
constexpr std::uint32_t kWriteRequestBinaryEncodingId = 671;
constexpr std::uint32_t kAnonymousIdentityTokenBinaryEncodingId = 321;
constexpr std::uint32_t kUserNameIdentityTokenBinaryEncodingId = 324;

enum class OpcUaBinaryTimestampsToReturn : std::uint32_t {
  Source = 0,
  Server = 1,
  Both = 2,
  Neither = 3,
};

using binary::ReadBoolean;
using binary::ReadByteString;
using binary::ReadDouble;
using binary::ReadExtensionObject;
using binary::ReadInt32;
using binary::ReadInt64;
using binary::ReadMessage;
using binary::ReadNumericNodeId;
using binary::ReadQualifiedName;
using binary::ReadUaString;
using binary::ReadUInt8;
using binary::ReadUInt32;

bool SkipStringArray(const std::vector<char>& bytes, std::size_t& offset) {
  std::int32_t count = 0;
  if (!ReadInt32(bytes, offset, count)) {
    return false;
  }
  if (count < 0) {
    return true;
  }
  for (std::int32_t i = 0; i < count; ++i) {
    std::string ignored;
    if (!ReadUaString(bytes, offset, ignored)) {
      return false;
    }
  }
  return true;
}

bool SkipLocalizedText(const std::vector<char>& bytes, std::size_t& offset) {
  std::uint8_t mask = 0;
  if (!ReadUInt8(bytes, offset, mask)) {
    return false;
  }
  if ((mask & 0x01) != 0) {
    std::string locale;
    if (!ReadUaString(bytes, offset, locale)) {
      return false;
    }
  }
  if ((mask & 0x02) != 0) {
    std::string text;
    if (!ReadUaString(bytes, offset, text)) {
      return false;
    }
  }
  return true;
}

bool SkipApplicationDescriptionFields(const std::vector<char>& bytes,
                                      std::size_t& offset) {
  std::string ignored;
  std::int32_t application_type = 0;
  return ReadUaString(bytes, offset, ignored) &&
         ReadUaString(bytes, offset, ignored) &&
         SkipLocalizedText(bytes, offset) &&
         ReadInt32(bytes, offset, application_type) &&
         ReadUaString(bytes, offset, ignored) &&
         ReadUaString(bytes, offset, ignored) &&
         SkipStringArray(bytes, offset);
}

bool SkipSignatureData(const std::vector<char>& bytes, std::size_t& offset) {
  std::string algorithm;
  scada::ByteString signature;
  return ReadUaString(bytes, offset, algorithm) &&
         ReadByteString(bytes, offset, signature);
}

bool SkipSignedSoftwareCertificates(const std::vector<char>& bytes,
                                    std::size_t& offset) {
  std::int32_t count = 0;
  if (!ReadInt32(bytes, offset, count)) {
    return false;
  }
  if (count < 0) {
    return true;
  }
  for (std::int32_t i = 0; i < count; ++i) {
    scada::ByteString certificate_data;
    scada::ByteString signature;
    if (!ReadByteString(bytes, offset, certificate_data) ||
        !ReadByteString(bytes, offset, signature)) {
      return false;
    }
  }
  return true;
}

bool ReadRequestHeader(const std::vector<char>& bytes,
                       std::size_t& offset,
                       OpcUaBinaryServiceRequestHeader& header) {
  std::int64_t ignored_timestamp = 0;
  if (!ReadNumericNodeId(bytes, offset, header.authentication_token) ||
      !ReadInt64(bytes, offset, ignored_timestamp) ||
      !ReadUInt32(bytes, offset, header.request_handle)) {
    return false;
  }

  std::uint32_t ignored_u32 = 0;
  std::string ignored_string;
  if (!ReadUInt32(bytes, offset, ignored_u32) ||
      !ReadUaString(bytes, offset, ignored_string) ||
      !ReadUInt32(bytes, offset, ignored_u32)) {
    return false;
  }

  std::uint32_t additional_type_id = 0;
  std::uint8_t additional_encoding = 0;
  std::vector<char> additional_body;
  return ReadExtensionObject(bytes, offset, additional_type_id,
                             additional_encoding, additional_body);
}

std::optional<scada::Variant> DecodeVariant(const std::vector<char>& bytes,
                                            std::size_t& offset) {
  std::uint8_t encoding_mask = 0;
  if (!ReadUInt8(bytes, offset, encoding_mask) ||
      (encoding_mask & 0x80) != 0) {
    return std::nullopt;
  }

  switch (encoding_mask) {
    case 0:
      return scada::Variant{};
    case 6: {
      std::int32_t value = 0;
      if (!ReadInt32(bytes, offset, value)) {
        return std::nullopt;
      }
      return scada::Variant{value};
    }
    case 7: {
      std::uint32_t value = 0;
      if (!ReadUInt32(bytes, offset, value)) {
        return std::nullopt;
      }
      return scada::Variant{value};
    }
    case 11: {
      double value = 0;
      if (!ReadDouble(bytes, offset, value)) {
        return std::nullopt;
      }
      return scada::Variant{value};
    }
    case 12: {
      std::string value;
      if (!ReadUaString(bytes, offset, value)) {
        return std::nullopt;
      }
      return scada::Variant{std::move(value)};
    }
    default:
      return std::nullopt;
  }
}

std::optional<scada::Variant> DecodeDataValue(const std::vector<char>& bytes,
                                              std::size_t& offset) {
  std::uint8_t mask = 0;
  if (!ReadUInt8(bytes, offset, mask) || (mask & 0xf0) != 0) {
    return std::nullopt;
  }

  scada::Variant value;
  if ((mask & 0x01) != 0) {
    auto decoded = DecodeVariant(bytes, offset);
    if (!decoded.has_value()) {
      return std::nullopt;
    }
    value = std::move(*decoded);
  }

  if ((mask & 0x02) != 0) {
    std::uint32_t ignored_status = 0;
    if (!ReadUInt32(bytes, offset, ignored_status)) {
      return std::nullopt;
    }
  }
  if ((mask & 0x04) != 0) {
    std::int64_t ignored_source_timestamp = 0;
    if (!ReadInt64(bytes, offset, ignored_source_timestamp)) {
      return std::nullopt;
    }
  }
  if ((mask & 0x08) != 0) {
    std::int64_t ignored_server_timestamp = 0;
    if (!ReadInt64(bytes, offset, ignored_server_timestamp)) {
      return std::nullopt;
    }
  }

  return value;
}

bool DecodeWriteValue(const std::vector<char>& bytes,
                      std::size_t& offset,
                      scada::WriteValue& value) {
  std::uint32_t attribute_id = 0;
  std::string index_range;
  if (!ReadNumericNodeId(bytes, offset, value.node_id) ||
      !ReadUInt32(bytes, offset, attribute_id) ||
      !ReadUaString(bytes, offset, index_range)) {
    return false;
  }

  auto decoded = DecodeDataValue(bytes, offset);
  if (!decoded.has_value()) {
    return false;
  }

  value.attribute_id = static_cast<scada::AttributeId>(attribute_id);
  value.value = std::move(*decoded);
  return index_range.empty();
}

bool DecodeReadValueId(const std::vector<char>& bytes,
                       std::size_t& offset,
                       scada::ReadValueId& value_id) {
  std::uint32_t attribute_id = 0;
  std::string index_range;
  scada::QualifiedName ignored_data_encoding;
  if (!ReadNumericNodeId(bytes, offset, value_id.node_id) ||
      !ReadUInt32(bytes, offset, attribute_id) ||
      !ReadUaString(bytes, offset, index_range) ||
      !ReadQualifiedName(bytes, offset, ignored_data_encoding)) {
    return false;
  }
  value_id.attribute_id = static_cast<scada::AttributeId>(attribute_id);
  return index_range.empty();
}

bool DecodeBrowseDescription(const std::vector<char>& bytes,
                             std::size_t& offset,
                             scada::BrowseDescription& description) {
  std::uint32_t browse_direction = 0;
  std::uint32_t ignored_node_class_mask = 0;
  std::uint32_t ignored_result_mask = 0;
  bool include_subtypes = false;
  if (!ReadNumericNodeId(bytes, offset, description.node_id) ||
      !ReadUInt32(bytes, offset, browse_direction) ||
      !ReadNumericNodeId(bytes, offset, description.reference_type_id) ||
      !ReadBoolean(bytes, offset, include_subtypes) ||
      !ReadUInt32(bytes, offset, ignored_node_class_mask) ||
      !ReadUInt32(bytes, offset, ignored_result_mask)) {
    return false;
  }
  description.direction =
      static_cast<scada::BrowseDirection>(browse_direction);
  description.include_subtypes = include_subtypes;
  return description.direction == scada::BrowseDirection::Forward ||
         description.direction == scada::BrowseDirection::Inverse ||
         description.direction == scada::BrowseDirection::Both;
}

std::optional<OpcUaBinaryDecodedRequest> DecodeCreateSessionRequest(
    const std::vector<char>& body) {
  std::size_t offset = 0;
  OpcUaBinaryServiceRequestHeader header;
  if (!ReadRequestHeader(body, offset, header) ||
      !SkipApplicationDescriptionFields(body, offset)) {
    return std::nullopt;
  }

  std::string ignored;
  scada::ByteString ignored_bytes;
  double requested_timeout_ms = 0;
  std::uint32_t ignored_max_response_size = 0;
  if (!ReadUaString(body, offset, ignored) ||
      !ReadUaString(body, offset, ignored) ||
      !ReadUaString(body, offset, ignored) ||
      !ReadByteString(body, offset, ignored_bytes) ||
      !ReadByteString(body, offset, ignored_bytes) ||
      !ReadDouble(body, offset, requested_timeout_ms) ||
      !ReadUInt32(body, offset, ignored_max_response_size) ||
      offset != body.size()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = OpcUaBinaryCreateSessionRequest{
          .requested_timeout =
              base::TimeDelta::FromMillisecondsD(requested_timeout_ms)},
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeActivateSessionRequest(
    const std::vector<char>& body) {
  std::size_t offset = 0;
  OpcUaBinaryServiceRequestHeader header;
  if (!ReadRequestHeader(body, offset, header) ||
      !header.authentication_token.is_numeric() ||
      !SkipSignatureData(body, offset) ||
      !SkipSignedSoftwareCertificates(body, offset) ||
      !SkipStringArray(body, offset)) {
    return std::nullopt;
  }

  std::uint32_t user_identity_type_id = 0;
  std::uint8_t user_identity_encoding = 0;
  std::vector<char> user_identity_body;
  if (!ReadExtensionObject(body, offset, user_identity_type_id,
                           user_identity_encoding, user_identity_body) ||
      user_identity_encoding != 0x01 ||
      !SkipSignatureData(body, offset) || offset != body.size()) {
    return std::nullopt;
  }

  OpcUaBinaryActivateSessionRequest request{
      .authentication_token = header.authentication_token,
  };

  std::size_t body_offset = 0;
  std::string ignored_policy_id;
  if (!ReadUaString(user_identity_body, body_offset, ignored_policy_id)) {
    return std::nullopt;
  }
  if (user_identity_type_id == kAnonymousIdentityTokenBinaryEncodingId) {
    request.allow_anonymous = true;
  } else if (user_identity_type_id == kUserNameIdentityTokenBinaryEncodingId) {
    std::string user_name;
    scada::ByteString password;
    std::string encryption_algorithm;
    if (!ReadUaString(user_identity_body, body_offset, user_name) ||
        !ReadByteString(user_identity_body, body_offset, password) ||
        !ReadUaString(user_identity_body, body_offset, encryption_algorithm) ||
        body_offset != user_identity_body.size() ||
        !encryption_algorithm.empty()) {
      return std::nullopt;
    }
    request.user_name = scada::ToLocalizedText(user_name);
    request.password = scada::ToLocalizedText(
        std::string{password.begin(), password.end()});
  } else {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeCloseSessionRequest(
    const std::vector<char>& body) {
  std::size_t offset = 0;
  OpcUaBinaryServiceRequestHeader header;
  std::uint8_t delete_subscriptions = 0;
  if (!ReadRequestHeader(body, offset, header) ||
      !ReadUInt8(body, offset, delete_subscriptions) ||
      offset != body.size()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = OpcUaBinaryCloseSessionRequest{
          .authentication_token = header.authentication_token,
      },
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeReadRequest(
    const std::vector<char>& body) {
  std::size_t offset = 0;
  OpcUaBinaryServiceRequestHeader header;
  double max_age = 0;
  std::uint32_t timestamps_to_return = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(body, offset, header) ||
      !ReadDouble(body, offset, max_age) ||
      !ReadUInt32(body, offset, timestamps_to_return) ||
      !ReadInt32(body, offset, count) || count < 0) {
    return std::nullopt;
  }
  if (max_age < 0) {
    return std::nullopt;
  }

  const auto timestamps =
      static_cast<OpcUaBinaryTimestampsToReturn>(timestamps_to_return);
  if (timestamps != OpcUaBinaryTimestampsToReturn::Source &&
      timestamps != OpcUaBinaryTimestampsToReturn::Server &&
      timestamps != OpcUaBinaryTimestampsToReturn::Both &&
      timestamps != OpcUaBinaryTimestampsToReturn::Neither) {
    return std::nullopt;
  }

  OpcUaBinaryReadRequest request;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeReadValueId(body, offset, input)) {
      return std::nullopt;
    }
  }
  if (offset != body.size()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeWriteRequest(
    const std::vector<char>& body) {
  std::size_t offset = 0;
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(body, offset, header) ||
      !ReadInt32(body, offset, count) || count < 0) {
    return std::nullopt;
  }

  OpcUaBinaryWriteRequest request;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeWriteValue(body, offset, input)) {
      return std::nullopt;
    }
  }
  if (offset != body.size()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeBrowseRequest(
    const std::vector<char>& body) {
  std::size_t offset = 0;
  OpcUaBinaryServiceRequestHeader header;
  scada::NodeId ignored_view_id;
  std::int64_t ignored_view_timestamp = 0;
  std::uint32_t ignored_view_version = 0;
  std::uint32_t requested_max_references_per_node = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(body, offset, header) ||
      !ReadNumericNodeId(body, offset, ignored_view_id) ||
      !ReadInt64(body, offset, ignored_view_timestamp) ||
      !ReadUInt32(body, offset, ignored_view_version) ||
      !ReadUInt32(body, offset, requested_max_references_per_node) ||
      !ReadInt32(body, offset, count) || count < 0) {
    return std::nullopt;
  }

  OpcUaBinaryBrowseRequest request;
  request.requested_max_references_per_node = requested_max_references_per_node;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeBrowseDescription(body, offset, input)) {
      return std::nullopt;
    }
  }
  if (offset != body.size()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

}  // namespace

std::optional<OpcUaBinaryDecodedRequest> DecodeOpcUaBinaryServiceRequest(
    const std::vector<char>& payload) {
  const auto message = ReadMessage(payload);
  if (!message.has_value()) {
    return std::nullopt;
  }

  switch (message->first) {
    case kCreateSessionRequestBinaryEncodingId:
      return DecodeCreateSessionRequest(message->second);
    case kActivateSessionRequestBinaryEncodingId:
      return DecodeActivateSessionRequest(message->second);
    case kCloseSessionRequestBinaryEncodingId:
      return DecodeCloseSessionRequest(message->second);
    case kBrowseRequestBinaryEncodingId:
      return DecodeBrowseRequest(message->second);
    case kReadRequestBinaryEncodingId:
      return DecodeReadRequest(message->second);
    case kWriteRequestBinaryEncodingId:
      return DecodeWriteRequest(message->second);
    default:
      return std::nullopt;
  }
}

}  // namespace opcua
