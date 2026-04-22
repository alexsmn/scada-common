#include "opcua/opcua_binary_service_codec.h"

#include "opcua/opcua_binary_codec_utils.h"

#include "scada/localized_text.h"

namespace opcua {
namespace {

constexpr std::uint32_t kCreateSessionRequestBinaryEncodingId = 461;
constexpr std::uint32_t kCreateSessionResponseBinaryEncodingId = 464;
constexpr std::uint32_t kActivateSessionRequestBinaryEncodingId = 467;
constexpr std::uint32_t kActivateSessionResponseBinaryEncodingId = 470;
constexpr std::uint32_t kCloseSessionRequestBinaryEncodingId = 473;
constexpr std::uint32_t kCloseSessionResponseBinaryEncodingId = 476;
constexpr std::uint32_t kBrowseRequestBinaryEncodingId = 525;
constexpr std::uint32_t kBrowseResponseBinaryEncodingId = 528;
constexpr std::uint32_t kCallRequestBinaryEncodingId = 710;
constexpr std::uint32_t kCallResponseBinaryEncodingId = 713;
constexpr std::uint32_t kReadRequestBinaryEncodingId = 629;
constexpr std::uint32_t kReadResponseBinaryEncodingId = 632;
constexpr std::uint32_t kWriteRequestBinaryEncodingId = 671;
constexpr std::uint32_t kWriteResponseBinaryEncodingId = 674;
constexpr std::uint32_t kAnonymousIdentityTokenBinaryEncodingId = 321;
constexpr std::uint32_t kUserNameIdentityTokenBinaryEncodingId = 324;

enum class WireTimestampsToReturn : std::uint32_t {
  Source = 0,
  Server = 1,
  Both = 2,
  Neither = 3,
};

void AppendRequestHeader(binary::BinaryEncoder& encoder,
                         const OpcUaBinaryServiceRequestHeader& header) {
  encoder.AppendNumericNodeId(header.authentication_token);
  encoder.AppendInt64(0);
  encoder.AppendUInt32(header.request_handle);
  encoder.AppendUInt32(0);
  encoder.AppendUaString("");
  encoder.AppendUInt32(0);
  encoder.AppendNumericNodeId(scada::NodeId{});
  encoder.AppendUInt8(0x00);
}

void AppendResponseHeader(binary::BinaryEncoder& encoder,
                          std::uint32_t request_handle,
                          scada::Status status) {
  encoder.AppendInt64(0);
  encoder.AppendUInt32(request_handle);
  encoder.AppendUInt32(status.full_code());
  encoder.AppendUInt8(0);
  encoder.AppendInt32(0);
  encoder.AppendNumericNodeId(scada::NodeId{});
  encoder.AppendUInt8(0x00);
}

std::uint32_t EncodeStatusCode(scada::StatusCode status_code) {
  return static_cast<std::uint32_t>(status_code) << 16;
}

void AppendDateTime(binary::BinaryEncoder& encoder, scada::DateTime value) {
  if (value.is_null()) {
    encoder.AppendInt64(0);
    return;
  }
  encoder.AppendInt64(value.ToDeltaSinceWindowsEpoch().InMicroseconds() * 10);
}

void AppendDataValue(binary::BinaryEncoder& encoder,
                     const scada::DataValue& value) {
  std::uint8_t mask = 0;
  if (!value.value.is_null()) {
    mask |= 0x01;
  }
  if (!scada::IsGood(value.status_code)) {
    mask |= 0x02;
  }
  if (!value.source_timestamp.is_null()) {
    mask |= 0x04;
  }
  if (!value.server_timestamp.is_null()) {
    mask |= 0x08;
  }

  encoder.AppendUInt8(mask);
  if ((mask & 0x01) != 0) {
    encoder.AppendVariant(value.value);
  }
  if ((mask & 0x02) != 0) {
    encoder.AppendUInt32(EncodeStatusCode(value.status_code));
  }
  if ((mask & 0x04) != 0) {
    AppendDateTime(encoder, value.source_timestamp);
  }
  if ((mask & 0x08) != 0) {
    AppendDateTime(encoder, value.server_timestamp);
  }
}

void AppendReferenceDescription(binary::BinaryEncoder& encoder,
                                const scada::ReferenceDescription& reference) {
  encoder.AppendNumericNodeId(reference.reference_type_id);
  encoder.AppendBoolean(reference.forward);
  encoder.AppendExpandedNodeId(scada::ExpandedNodeId{reference.node_id});
  encoder.AppendQualifiedName(scada::QualifiedName{});
  encoder.AppendLocalizedText(scada::LocalizedText{});
  encoder.AppendUInt32(0);
  encoder.AppendExpandedNodeId(scada::ExpandedNodeId{});
}

bool SkipStringArray(binary::BinaryDecoder& decoder) {
  std::int32_t count = 0;
  if (!decoder.ReadInt32(count)) {
    return false;
  }
  if (count < 0) {
    return true;
  }
  for (std::int32_t i = 0; i < count; ++i) {
    std::string ignored;
    if (!decoder.ReadUaString(ignored)) {
      return false;
    }
  }
  return true;
}

bool SkipLocalizedText(binary::BinaryDecoder& decoder) {
  std::uint8_t mask = 0;
  if (!decoder.ReadUInt8(mask)) {
    return false;
  }
  if ((mask & 0x01) != 0) {
    std::string locale;
    if (!decoder.ReadUaString(locale)) {
      return false;
    }
  }
  if ((mask & 0x02) != 0) {
    std::string text;
    if (!decoder.ReadUaString(text)) {
      return false;
    }
  }
  return true;
}

bool SkipApplicationDescriptionFields(binary::BinaryDecoder& decoder) {
  std::string ignored;
  std::int32_t application_type = 0;
  return decoder.ReadUaString(ignored) && decoder.ReadUaString(ignored) &&
         SkipLocalizedText(decoder) && decoder.ReadInt32(application_type) &&
         decoder.ReadUaString(ignored) && decoder.ReadUaString(ignored) &&
         SkipStringArray(decoder);
}

bool SkipSignatureData(binary::BinaryDecoder& decoder) {
  std::string algorithm;
  scada::ByteString signature;
  return decoder.ReadUaString(algorithm) && decoder.ReadByteString(signature);
}

bool SkipSignedSoftwareCertificates(binary::BinaryDecoder& decoder) {
  std::int32_t count = 0;
  if (!decoder.ReadInt32(count)) {
    return false;
  }
  if (count < 0) {
    return true;
  }
  for (std::int32_t i = 0; i < count; ++i) {
    scada::ByteString certificate_data;
    scada::ByteString signature;
    if (!decoder.ReadByteString(certificate_data) ||
        !decoder.ReadByteString(signature)) {
      return false;
    }
  }
  return true;
}

bool ReadRequestHeader(binary::BinaryDecoder& decoder,
                       OpcUaBinaryServiceRequestHeader& header) {
  std::int64_t ignored_timestamp = 0;
  if (!decoder.ReadNumericNodeId(header.authentication_token) ||
      !decoder.ReadInt64(ignored_timestamp) ||
      !decoder.ReadUInt32(header.request_handle)) {
    return false;
  }

  std::uint32_t ignored_u32 = 0;
  std::string ignored_string;
  if (!decoder.ReadUInt32(ignored_u32) || !decoder.ReadUaString(ignored_string) ||
      !decoder.ReadUInt32(ignored_u32)) {
    return false;
  }

  std::uint32_t additional_type_id = 0;
  std::uint8_t additional_encoding = 0;
  std::vector<char> additional_body;
  return decoder.ReadExtensionObject(additional_type_id, additional_encoding,
                                     additional_body);
}

std::optional<scada::Variant> DecodeDataValue(binary::BinaryDecoder& decoder) {
  std::uint8_t mask = 0;
  if (!decoder.ReadUInt8(mask) || (mask & 0xf0) != 0) {
    return std::nullopt;
  }

  scada::Variant value;
  if ((mask & 0x01) != 0) {
    if (!decoder.ReadVariant(value)) {
      return std::nullopt;
    }
  }

  if ((mask & 0x02) != 0) {
    std::uint32_t ignored_status = 0;
    if (!decoder.ReadUInt32(ignored_status)) {
      return std::nullopt;
    }
  }
  if ((mask & 0x04) != 0) {
    std::int64_t ignored_source_timestamp = 0;
    if (!decoder.ReadInt64(ignored_source_timestamp)) {
      return std::nullopt;
    }
  }
  if ((mask & 0x08) != 0) {
    std::int64_t ignored_server_timestamp = 0;
    if (!decoder.ReadInt64(ignored_server_timestamp)) {
      return std::nullopt;
    }
  }

  return value;
}

bool DecodeWriteValue(binary::BinaryDecoder& decoder,
                      scada::WriteValue& value) {
  std::uint32_t attribute_id = 0;
  std::string index_range;
  if (!decoder.ReadNumericNodeId(value.node_id) ||
      !decoder.ReadUInt32(attribute_id) ||
      !decoder.ReadUaString(index_range)) {
    return false;
  }

  auto decoded = DecodeDataValue(decoder);
  if (!decoded.has_value()) {
    return false;
  }

  value.attribute_id = static_cast<scada::AttributeId>(attribute_id);
  value.value = std::move(*decoded);
  return index_range.empty();
}

bool DecodeReadValueId(binary::BinaryDecoder& decoder,
                       scada::ReadValueId& value_id) {
  std::uint32_t attribute_id = 0;
  std::string index_range;
  scada::QualifiedName ignored_data_encoding;
  if (!decoder.ReadNumericNodeId(value_id.node_id) ||
      !decoder.ReadUInt32(attribute_id) ||
      !decoder.ReadUaString(index_range) ||
      !decoder.ReadQualifiedName(ignored_data_encoding)) {
    return false;
  }
  value_id.attribute_id = static_cast<scada::AttributeId>(attribute_id);
  return index_range.empty();
}

bool DecodeBrowseDescription(binary::BinaryDecoder& decoder,
                             scada::BrowseDescription& description) {
  std::uint32_t browse_direction = 0;
  std::uint32_t ignored_node_class_mask = 0;
  std::uint32_t ignored_result_mask = 0;
  bool include_subtypes = false;
  if (!decoder.ReadNumericNodeId(description.node_id) ||
      !decoder.ReadUInt32(browse_direction) ||
      !decoder.ReadNumericNodeId(description.reference_type_id) ||
      !decoder.ReadBoolean(include_subtypes) ||
      !decoder.ReadUInt32(ignored_node_class_mask) ||
      !decoder.ReadUInt32(ignored_result_mask)) {
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
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  if (!ReadRequestHeader(decoder, header) ||
      !SkipApplicationDescriptionFields(decoder)) {
    return std::nullopt;
  }

  std::string ignored;
  scada::ByteString ignored_bytes;
  double requested_timeout_ms = 0;
  std::uint32_t ignored_max_response_size = 0;
  if (!decoder.ReadUaString(ignored) || !decoder.ReadUaString(ignored) ||
      !decoder.ReadUaString(ignored) || !decoder.ReadByteString(ignored_bytes) ||
      !decoder.ReadByteString(ignored_bytes) ||
      !decoder.ReadDouble(requested_timeout_ms) ||
      !decoder.ReadUInt32(ignored_max_response_size) || !decoder.consumed()) {
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
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  if (!ReadRequestHeader(decoder, header) ||
      !header.authentication_token.is_numeric() ||
      !SkipSignatureData(decoder) ||
      !SkipSignedSoftwareCertificates(decoder) ||
      !SkipStringArray(decoder)) {
    return std::nullopt;
  }

  std::uint32_t user_identity_type_id = 0;
  std::uint8_t user_identity_encoding = 0;
  std::vector<char> user_identity_body;
  if (!decoder.ReadExtensionObject(user_identity_type_id, user_identity_encoding,
                                   user_identity_body) ||
      user_identity_encoding != 0x01 ||
      !SkipSignatureData(decoder) || !decoder.consumed()) {
    return std::nullopt;
  }

  OpcUaBinaryActivateSessionRequest request{
      .authentication_token = header.authentication_token,
  };

  binary::BinaryDecoder body_decoder{user_identity_body};
  std::string ignored_policy_id;
  if (!body_decoder.ReadUaString(ignored_policy_id)) {
    return std::nullopt;
  }
  if (user_identity_type_id == kAnonymousIdentityTokenBinaryEncodingId) {
    request.allow_anonymous = true;
  } else if (user_identity_type_id == kUserNameIdentityTokenBinaryEncodingId) {
    std::string user_name;
    scada::ByteString password;
    std::string encryption_algorithm;
    if (!body_decoder.ReadUaString(user_name) ||
        !body_decoder.ReadByteString(password) ||
        !body_decoder.ReadUaString(encryption_algorithm) ||
        !body_decoder.consumed() ||
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
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::uint8_t delete_subscriptions = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.ReadUInt8(delete_subscriptions) || !decoder.consumed()) {
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
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  double max_age = 0;
  std::uint32_t timestamps_to_return = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.ReadDouble(max_age) ||
      !decoder.ReadUInt32(timestamps_to_return) || !decoder.ReadInt32(count) ||
      count < 0) {
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
    if (!DecodeReadValueId(decoder, input)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeWriteRequest(
    const std::vector<char>& body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.ReadInt32(count) ||
      count < 0) {
    return std::nullopt;
  }

  OpcUaBinaryWriteRequest request;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeWriteValue(decoder, input)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeBrowseRequest(
    const std::vector<char>& body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  scada::NodeId ignored_view_id;
  std::int64_t ignored_view_timestamp = 0;
  std::uint32_t ignored_view_version = 0;
  std::uint32_t requested_max_references_per_node = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.ReadNumericNodeId(ignored_view_id) ||
      !decoder.ReadInt64(ignored_view_timestamp) ||
      !decoder.ReadUInt32(ignored_view_version) ||
      !decoder.ReadUInt32(requested_max_references_per_node) ||
      !decoder.ReadInt32(count) || count < 0) {
    return std::nullopt;
  }

  OpcUaBinaryBrowseRequest request;
  request.requested_max_references_per_node = requested_max_references_per_node;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeBrowseDescription(decoder, input)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeCallRequest(
    const std::vector<char>& body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.ReadInt32(count) ||
      count < 0) {
    return std::nullopt;
  }

  OpcUaBinaryCallRequest request;
  request.methods.resize(static_cast<std::size_t>(count));
  for (auto& method : request.methods) {
    std::int32_t argument_count = 0;
    if (!decoder.ReadNumericNodeId(method.object_id) ||
        !decoder.ReadNumericNodeId(method.method_id) ||
        !decoder.ReadInt32(argument_count) || argument_count < 0) {
      return std::nullopt;
    }

    method.arguments.reserve(static_cast<std::size_t>(argument_count));
    for (std::int32_t i = 0; i < argument_count; ++i) {
      scada::Variant argument;
      if (!decoder.ReadVariant(argument)) {
        return std::nullopt;
      }
      method.arguments.push_back(std::move(argument));
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

}  // namespace

std::optional<std::vector<char>> EncodeOpcUaBinaryServiceRequest(
    const OpcUaBinaryServiceRequestHeader& header,
    const OpcUaBinaryRequestBody& request) {
  return std::visit(
      [&](const auto& typed_request) -> std::optional<std::vector<char>> {
        std::vector<char> payload;
        std::vector<char> body;
        binary::BinaryEncoder payload_encoder{payload};
        binary::BinaryEncoder body_encoder{body};

        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, OpcUaBinaryCreateSessionRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.AppendUaString("");
          payload_encoder.AppendUaString("");
          payload_encoder.AppendUInt8(0);
          payload_encoder.AppendInt32(0);
          payload_encoder.AppendUaString("");
          payload_encoder.AppendUaString("");
          payload_encoder.AppendInt32(-1);
          payload_encoder.AppendUaString("");
          payload_encoder.AppendUaString("opc.tcp://localhost:4840");
          payload_encoder.AppendUaString("binary-session");
          payload_encoder.AppendByteString({});
          payload_encoder.AppendByteString({});
          payload_encoder.AppendDouble(
              typed_request.requested_timeout.InMillisecondsF());
          payload_encoder.AppendUInt32(0);
          body_encoder.AppendMessage(kCreateSessionRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryActivateSessionRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.AppendUaString("");
          payload_encoder.AppendByteString({});
          payload_encoder.AppendInt32(-1);
          payload_encoder.AppendInt32(-1);
          if (typed_request.allow_anonymous) {
            std::vector<char> identity;
            binary::BinaryEncoder identity_encoder{identity};
            identity_encoder.AppendUaString("");
            payload_encoder.AppendExtensionObject(
                kAnonymousIdentityTokenBinaryEncodingId, identity);
          } else {
            std::vector<char> identity;
            binary::BinaryEncoder identity_encoder{identity};
            identity_encoder.AppendUaString("");
            identity_encoder.AppendUaString(
                typed_request.user_name.has_value()
                    ? ToString(*typed_request.user_name)
                    : std::string{});
            const auto password =
                typed_request.password.has_value()
                    ? ToString(*typed_request.password)
                    : std::string{};
            identity_encoder.AppendByteString(
                scada::ByteString{password.begin(), password.end()});
            identity_encoder.AppendUaString("");
            payload_encoder.AppendExtensionObject(
                kUserNameIdentityTokenBinaryEncodingId, identity);
          }
          payload_encoder.AppendUaString("");
          payload_encoder.AppendByteString({});
          body_encoder.AppendMessage(kActivateSessionRequestBinaryEncodingId,
                                     payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryCloseSessionRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.AppendUInt8(1);
          body_encoder.AppendMessage(kCloseSessionRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryReadRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.AppendDouble(0);
          payload_encoder.AppendUInt32(
              static_cast<std::uint32_t>(WireTimestampsToReturn::Both));
          payload_encoder.AppendInt32(
              static_cast<std::int32_t>(typed_request.inputs.size()));
          for (const auto& input : typed_request.inputs) {
            payload_encoder.AppendNumericNodeId(input.node_id);
            payload_encoder.AppendUInt32(
                static_cast<std::uint32_t>(input.attribute_id));
            payload_encoder.AppendUaString("");
            payload_encoder.AppendUInt16(0);
            payload_encoder.AppendUaString("");
          }
          body_encoder.AppendMessage(kReadRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryWriteRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.AppendInt32(
              static_cast<std::int32_t>(typed_request.inputs.size()));
          for (const auto& input : typed_request.inputs) {
            payload_encoder.AppendNumericNodeId(input.node_id);
            payload_encoder.AppendUInt32(
                static_cast<std::uint32_t>(input.attribute_id));
            payload_encoder.AppendUaString("");
            payload_encoder.AppendUInt8(0x01);
            payload_encoder.AppendVariant(input.value);
          }
          body_encoder.AppendMessage(kWriteRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryBrowseRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.AppendNumericNodeId(scada::NodeId{});
          payload_encoder.AppendInt64(0);
          payload_encoder.AppendUInt32(0);
          payload_encoder.AppendUInt32(
              typed_request.requested_max_references_per_node);
          payload_encoder.AppendInt32(
              static_cast<std::int32_t>(typed_request.inputs.size()));
          for (const auto& input : typed_request.inputs) {
            payload_encoder.AppendNumericNodeId(input.node_id);
            payload_encoder.AppendUInt32(
                static_cast<std::uint32_t>(input.direction));
            payload_encoder.AppendNumericNodeId(input.reference_type_id);
            payload_encoder.AppendBoolean(input.include_subtypes);
            payload_encoder.AppendUInt32(0);
            payload_encoder.AppendUInt32(0);
          }
          body_encoder.AppendMessage(kBrowseRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryCallRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.AppendInt32(
              static_cast<std::int32_t>(typed_request.methods.size()));
          for (const auto& method : typed_request.methods) {
            payload_encoder.AppendNumericNodeId(method.object_id);
            payload_encoder.AppendNumericNodeId(method.method_id);
            payload_encoder.AppendInt32(
                static_cast<std::int32_t>(method.arguments.size()));
            for (const auto& argument : method.arguments) {
              payload_encoder.AppendVariant(argument);
            }
          }
          body_encoder.AppendMessage(kCallRequestBinaryEncodingId, payload);
        } else {
          return std::nullopt;
        }

        return body;
      },
      request);
}

std::optional<OpcUaBinaryDecodedRequest> DecodeOpcUaBinaryServiceRequest(
    const std::vector<char>& payload) {
  binary::BinaryDecoder decoder{payload};
  const auto message = binary::ReadMessage(payload);
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
    case kCallRequestBinaryEncodingId:
      return DecodeCallRequest(message->second);
    case kReadRequestBinaryEncodingId:
      return DecodeReadRequest(message->second);
    case kWriteRequestBinaryEncodingId:
      return DecodeWriteRequest(message->second);
    default:
      return std::nullopt;
  }
}

std::optional<std::vector<char>> EncodeOpcUaBinaryServiceResponse(
    std::uint32_t request_handle,
    const OpcUaBinaryResponseBody& response) {
  return std::visit(
      [&](const auto& typed_response) -> std::optional<std::vector<char>> {
        std::vector<char> payload;
        std::vector<char> body;
        binary::BinaryEncoder payload_encoder{payload};
        binary::BinaryEncoder body_encoder{body};

        using T = std::decay_t<decltype(typed_response)>;
        if constexpr (std::is_same_v<T, OpcUaBinaryCreateSessionResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.AppendNumericNodeId(typed_response.session_id);
          payload_encoder.AppendNumericNodeId(
              typed_response.authentication_token);
          payload_encoder.AppendDouble(
              typed_response.revised_timeout.InMillisecondsF());
          payload_encoder.AppendByteString(typed_response.server_nonce);
          payload_encoder.AppendByteString({});
          payload_encoder.AppendInt32(-1);
          payload_encoder.AppendInt32(-1);
          payload_encoder.AppendUaString("");
          payload_encoder.AppendByteString({});
          payload_encoder.AppendUInt32(0);
          body_encoder.AppendMessage(kCreateSessionResponseBinaryEncodingId,
                                     payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryActivateSessionResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.AppendByteString({});
          payload_encoder.AppendInt32(-1);
          payload_encoder.AppendInt32(-1);
          body_encoder.AppendMessage(kActivateSessionResponseBinaryEncodingId,
                                     payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryCloseSessionResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          body_encoder.AppendMessage(kCloseSessionResponseBinaryEncodingId,
                                     payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryReadResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.AppendInt32(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            AppendDataValue(payload_encoder, result);
          }
          payload_encoder.AppendInt32(-1);
          body_encoder.AppendMessage(kReadResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryWriteResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.AppendInt32(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.AppendUInt32(EncodeStatusCode(result));
          }
          payload_encoder.AppendInt32(-1);
          body_encoder.AppendMessage(kWriteResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryBrowseResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.AppendInt32(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.AppendUInt32(EncodeStatusCode(result.status_code));
            payload_encoder.AppendByteString(result.continuation_point);
            payload_encoder.AppendInt32(
                static_cast<std::int32_t>(result.references.size()));
            for (const auto& reference : result.references) {
              AppendReferenceDescription(payload_encoder, reference);
            }
          }
          payload_encoder.AppendInt32(-1);
          body_encoder.AppendMessage(kBrowseResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaBinaryCallResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               scada::StatusCode::Good);
          payload_encoder.AppendInt32(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.AppendUInt32(result.status.full_code());
            payload_encoder.AppendInt32(static_cast<std::int32_t>(
                result.input_argument_results.size()));
            for (const auto& input_result : result.input_argument_results) {
              payload_encoder.AppendUInt32(EncodeStatusCode(input_result));
            }
            payload_encoder.AppendInt32(-1);
            payload_encoder.AppendInt32(
                static_cast<std::int32_t>(result.output_arguments.size()));
            for (const auto& output_argument : result.output_arguments) {
              payload_encoder.AppendVariant(output_argument);
            }
          }
          payload_encoder.AppendInt32(-1);
          body_encoder.AppendMessage(kCallResponseBinaryEncodingId, payload);
        } else {
          return std::nullopt;
        }

        return body;
      },
      response);
}

}  // namespace opcua
