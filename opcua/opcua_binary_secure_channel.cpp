#include "opcua/opcua_binary_secure_channel.h"
#include "opcua/opcua_binary_codec_utils.h"

#include <cstring>

namespace opcua {
namespace {

void AppendNumericNodeId(binary::BinaryEncoder& encoder, std::uint32_t id) {
  encoder.AppendNumericNodeId(scada::NodeId{id});
}

bool ReadNumericNodeId(binary::BinaryDecoder& decoder, std::uint32_t& id) {
  scada::NodeId node_id;
  if (!decoder.ReadNumericNodeId(node_id) || !node_id.is_numeric() ||
      node_id.namespace_index() != 0) {
    return false;
  }
  id = node_id.numeric_id();
  return true;
}

void AppendExtensionObject(binary::BinaryEncoder& encoder,
                           std::uint32_t type_id,
                           const std::vector<char>& body) {
  encoder.AppendExtensionObject(type_id, body);
}

bool ReadExtensionObject(binary::BinaryDecoder& decoder,
                         std::uint32_t& type_id,
                         std::uint8_t& encoding,
                         std::vector<char>& body) {
  return decoder.ReadExtensionObject(type_id, encoding, body);
}

bool ReadRequestHeader(binary::BinaryDecoder& decoder,
                       OpcUaBinaryRequestHeader& header) {
  std::uint32_t ignored_node_id = 0;
  std::int64_t ignored_timestamp = 0;
  if (!ReadNumericNodeId(decoder, ignored_node_id) ||
      !decoder.ReadInt64(ignored_timestamp) ||
      !decoder.ReadUInt32(header.request_handle) ||
      !decoder.ReadUInt32(header.return_diagnostics) ||
      !decoder.ReadUaString(header.audit_entry_id) ||
      !decoder.ReadUInt32(header.timeout_hint)) {
    return false;
  }

  std::uint32_t additional_type_id = 0;
  std::uint8_t additional_encoding = 0;
  std::vector<char> additional_body;
  return ReadExtensionObject(decoder, additional_type_id, additional_encoding,
                             additional_body);
}

void AppendResponseHeader(binary::BinaryEncoder& encoder,
                          const OpcUaBinaryResponseHeader& header) {
  encoder.AppendInt64(0);
  encoder.AppendUInt32(header.request_handle);
  encoder.AppendUInt32(header.service_result.good() ? 0u : 0x80000000u);
  encoder.AppendUInt8(0);
  encoder.AppendInt32(0);
  AppendNumericNodeId(encoder, 0);
  encoder.AppendUInt8(0x00);
}

void AppendRequestHeader(binary::BinaryEncoder& encoder,
                         const OpcUaBinaryRequestHeader& header) {
  AppendNumericNodeId(encoder, 0);
  encoder.AppendInt64(0);
  encoder.AppendUInt32(header.request_handle);
  encoder.AppendUInt32(header.return_diagnostics);
  encoder.AppendUaString(header.audit_entry_id);
  encoder.AppendUInt32(header.timeout_hint);
  AppendNumericNodeId(encoder, 0);
  encoder.AppendUInt8(0x00);
}

}  // namespace

std::optional<OpcUaBinarySecureConversationMessage>
DecodeSecureConversationMessage(const std::vector<char>& frame) {
  const auto frame_header = DecodeBinaryFrameHeader(frame);
  if (!frame_header || frame_header->message_size != frame.size()) {
    return std::nullopt;
  }

  OpcUaBinarySecureConversationMessage message;
  message.frame_header = *frame_header;
  binary::BinaryDecoder decoder{std::span<const char>{frame}.subspan(8)};
  if (!decoder.ReadUInt32(message.secure_channel_id)) {
    return std::nullopt;
  }

  if (frame_header->message_type == OpcUaBinaryMessageType::SecureOpen) {
    OpcUaBinaryAsymmetricSecurityHeader header;
    if (!decoder.ReadUaString(header.security_policy_uri) ||
        !decoder.ReadByteString(header.sender_certificate) ||
        !decoder.ReadByteString(
                        header.receiver_certificate_thumbprint)) {
      return std::nullopt;
    }
    message.asymmetric_security_header = std::move(header);
  } else {
    OpcUaBinarySymmetricSecurityHeader header;
    if (!decoder.ReadUInt32(header.token_id)) {
      return std::nullopt;
    }
    message.symmetric_security_header = header;
  }

  if (!decoder.ReadUInt32(message.sequence_header.sequence_number) ||
      !decoder.ReadUInt32(message.sequence_header.request_id)) {
    return std::nullopt;
  }

  message.body.assign(frame.begin() + static_cast<std::ptrdiff_t>(8 + decoder.offset()),
                      frame.end());
  return message;
}

std::vector<char> EncodeSecureConversationMessage(
    const OpcUaBinarySecureConversationMessage& message) {
  std::vector<char> frame = EncodeBinaryFrameHeader(
      {.message_type = message.frame_header.message_type,
       .chunk_type = message.frame_header.chunk_type,
       .message_size = 0});
  binary::BinaryEncoder encoder{frame};
  encoder.AppendUInt32(message.secure_channel_id);

  if (message.frame_header.message_type == OpcUaBinaryMessageType::SecureOpen) {
    const auto& header = *message.asymmetric_security_header;
    encoder.AppendUaString(header.security_policy_uri);
    encoder.AppendByteString(header.sender_certificate);
    encoder.AppendByteString(header.receiver_certificate_thumbprint);
  } else {
    encoder.AppendUInt32(message.symmetric_security_header->token_id);
  }

  encoder.AppendUInt32(message.sequence_header.sequence_number);
  encoder.AppendUInt32(message.sequence_header.request_id);
  frame.insert(frame.end(), message.body.begin(), message.body.end());

  const auto message_size = static_cast<std::uint32_t>(frame.size());
  std::memcpy(frame.data() + 4, &message_size, sizeof(message_size));
  return frame;
}

std::optional<OpcUaBinaryOpenSecureChannelRequest>
DecodeOpenSecureChannelRequestBody(const std::vector<char>& body) {
  binary::BinaryDecoder body_decoder{body};
  std::uint32_t type_id = 0;
  std::uint8_t encoding = 0;
  std::vector<char> payload;
  if (!ReadExtensionObject(body_decoder, type_id, encoding, payload) ||
      type_id != kOpenSecureChannelRequestBinaryEncodingId || encoding != 0x01 ||
      !body_decoder.consumed()) {
    return std::nullopt;
  }

  OpcUaBinaryOpenSecureChannelRequest request;
  binary::BinaryDecoder payload_decoder{payload};
  std::uint32_t request_type = 0;
  std::uint32_t security_mode = 0;
  if (!ReadRequestHeader(payload_decoder, request.request_header) ||
      !payload_decoder.ReadUInt32(request.client_protocol_version) ||
      !payload_decoder.ReadUInt32(request_type) ||
      !payload_decoder.ReadUInt32(security_mode) ||
      !payload_decoder.ReadByteString(request.client_nonce) ||
      !payload_decoder.ReadUInt32(request.requested_lifetime) ||
      !payload_decoder.consumed()) {
    return std::nullopt;
  }

  request.request_type =
      static_cast<OpcUaBinarySecurityTokenRequestType>(request_type);
  request.security_mode =
      static_cast<OpcUaBinaryMessageSecurityMode>(security_mode);
  return request;
}

std::vector<char> EncodeOpenSecureChannelResponseBody(
    const OpcUaBinaryOpenSecureChannelResponse& response) {
  std::vector<char> payload;
  binary::BinaryEncoder payload_encoder{payload};
  AppendResponseHeader(payload_encoder, response.response_header);
  payload_encoder.AppendUInt32(response.server_protocol_version);
  payload_encoder.AppendUInt32(response.security_token.channel_id);
  payload_encoder.AppendUInt32(response.security_token.token_id);
  payload_encoder.AppendInt64(response.security_token.created_at);
  payload_encoder.AppendUInt32(response.security_token.revised_lifetime);
  payload_encoder.AppendByteString(response.server_nonce);

  std::vector<char> body;
  binary::BinaryEncoder body_encoder{body};
  AppendExtensionObject(body_encoder,
                        kOpenSecureChannelResponseBinaryEncodingId, payload);
  return body;
}

std::optional<OpcUaBinaryCloseSecureChannelRequest>
DecodeCloseSecureChannelRequestBody(const std::vector<char>& body) {
  binary::BinaryDecoder body_decoder{body};
  std::uint32_t type_id = 0;
  std::uint8_t encoding = 0;
  std::vector<char> payload;
  if (!ReadExtensionObject(body_decoder, type_id, encoding, payload) ||
      type_id != kCloseSecureChannelRequestBinaryEncodingId || encoding != 0x01 ||
      !body_decoder.consumed()) {
    return std::nullopt;
  }

  OpcUaBinaryCloseSecureChannelRequest request;
  binary::BinaryDecoder payload_decoder{payload};
  if (!ReadRequestHeader(payload_decoder, request.request_header) ||
      !payload_decoder.consumed()) {
    return std::nullopt;
  }
  return request;
}

OpcUaBinarySecureChannel::OpcUaBinarySecureChannel(std::uint32_t channel_id)
    : channel_id_{channel_id} {}

Awaitable<OpcUaBinarySecureChannel::Result> OpcUaBinarySecureChannel::HandleFrame(
    std::vector<char> frame) {
  const auto message = DecodeSecureConversationMessage(frame);
  if (!message.has_value()) {
    co_return Result{.close_transport = true};
  }

  switch (message->frame_header.message_type) {
    case OpcUaBinaryMessageType::SecureOpen: {
      const auto request = DecodeOpenSecureChannelRequestBody(message->body);
      if (!request.has_value()) {
        co_return Result{.close_transport = true};
      }

      const auto supported_security =
          request->security_mode == OpcUaBinaryMessageSecurityMode::None &&
          message->asymmetric_security_header &&
          message->asymmetric_security_header->security_policy_uri ==
              kSecurityPolicyNone;

      auto response = BuildOpenResponse(
          *message, *request,
          supported_security ? scada::StatusCode::Good : scada::StatusCode::Bad);
      if (supported_security) {
        opened_ = true;
        if (request->request_type == OpcUaBinarySecurityTokenRequestType::Renew) {
          ++token_id_;
        }
      }
      co_return Result{.outbound_frame = std::move(response)};
    }

    case OpcUaBinaryMessageType::SecureMessage: {
      if (!opened_ || message->secure_channel_id != channel_id_ ||
          !message->symmetric_security_header ||
          message->symmetric_security_header->token_id != token_id_) {
        co_return Result{.close_transport = true};
      }
      co_return Result{.service_payload = std::move(message->body),
                       .request_id = message->sequence_header.request_id};
    }

    case OpcUaBinaryMessageType::SecureClose: {
      if (!opened_ || message->secure_channel_id != channel_id_ ||
          !message->symmetric_security_header ||
          message->symmetric_security_header->token_id != token_id_) {
        co_return Result{.close_transport = true};
      }
      const auto request = DecodeCloseSecureChannelRequestBody(message->body);
      opened_ = false;
      co_return Result{.close_transport = !request.has_value() || true};
    }

    case OpcUaBinaryMessageType::Hello:
    case OpcUaBinaryMessageType::Acknowledge:
    case OpcUaBinaryMessageType::Error:
    case OpcUaBinaryMessageType::ReverseHello:
      co_return Result{.close_transport = true};
  }

  co_return Result{.close_transport = true};
}

std::vector<char> OpcUaBinarySecureChannel::BuildServiceResponse(
    std::uint32_t request_id,
    std::vector<char> body) {
  OpcUaBinarySecureConversationMessage message{
      .frame_header =
          {.message_type = OpcUaBinaryMessageType::SecureMessage,
           .chunk_type = 'F',
           .message_size = 0},
      .secure_channel_id = channel_id_,
      .symmetric_security_header =
          OpcUaBinarySymmetricSecurityHeader{.token_id = token_id_},
      .sequence_header =
          {.sequence_number = next_sequence_number_++, .request_id = request_id},
      .body = std::move(body),
  };
  return EncodeSecureConversationMessage(message);
}

std::vector<char> OpcUaBinarySecureChannel::BuildOpenResponse(
    const OpcUaBinarySecureConversationMessage& request_message,
    const OpcUaBinaryOpenSecureChannelRequest& request,
    scada::Status service_result) {
  OpcUaBinaryOpenSecureChannelResponse response{
      .response_header = {.request_handle = request.request_header.request_handle,
                          .service_result = service_result},
      .server_protocol_version = request.client_protocol_version,
      .security_token =
          {.channel_id = channel_id_,
           .token_id = token_id_,
           .created_at = 0,
           .revised_lifetime = request.requested_lifetime},
      .server_nonce = {},
  };

  OpcUaBinarySecureConversationMessage message{
      .frame_header =
          {.message_type = OpcUaBinaryMessageType::SecureOpen,
           .chunk_type = 'F',
           .message_size = 0},
      .secure_channel_id = channel_id_,
      .asymmetric_security_header = OpcUaBinaryAsymmetricSecurityHeader{
          .security_policy_uri = std::string{kSecurityPolicyNone},
          .sender_certificate = {},
          .receiver_certificate_thumbprint = {},
      },
      .sequence_header =
          {.sequence_number = next_sequence_number_++,
           .request_id = request_message.sequence_header.request_id},
      .body = EncodeOpenSecureChannelResponseBody(response),
  };
  return EncodeSecureConversationMessage(message);
}

}  // namespace opcua
