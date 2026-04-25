#include "opcua/binary/secure_channel.h"
#include "opcua/binary/codec_utils.h"

#include <cstring>

namespace opcua::binary {
namespace {

void AppendNumericNodeId(Encoder& encoder, std::uint32_t id) {
  encoder.Encode(scada::NodeId{id});
}

bool ReadNumericNodeId(Decoder& decoder, std::uint32_t& id) {
  scada::NodeId node_id;
  if (!decoder.Decode(node_id) || !node_id.is_numeric() ||
      node_id.namespace_index() != 0) {
    return false;
  }
  id = node_id.numeric_id();
  return true;
}

void AppendExtensionObject(Encoder& encoder,
                           std::uint32_t type_id,
                           const std::vector<char>& body) {
  encoder.Encode(EncodedExtensionObject{.type_id = type_id, .body = body});
}

bool ReadExtensionObject(Decoder& decoder,
                         std::uint32_t& type_id,
                         std::uint8_t& encoding,
                         std::vector<char>& body) {
  DecodedExtensionObject value;
  if (!decoder.Decode(value)) {
    return false;
  }
  type_id = value.type_id;
  encoding = value.encoding;
  body = std::move(value.body);
  return true;
}

bool ReadRequestHeader(Decoder& decoder,
                       RequestHeader& header) {
  std::uint32_t ignored_node_id = 0;
  std::int64_t ignored_timestamp = 0;
  if (!ReadNumericNodeId(decoder, ignored_node_id) ||
      !decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(header.request_handle) ||
      !decoder.Decode(header.return_diagnostics) ||
      !decoder.Decode(header.audit_entry_id) ||
      !decoder.Decode(header.timeout_hint)) {
    return false;
  }

  std::uint32_t additional_type_id = 0;
  std::uint8_t additional_encoding = 0;
  std::vector<char> additional_body;
  return ReadExtensionObject(decoder, additional_type_id, additional_encoding,
                             additional_body);
}

void AppendResponseHeader(Encoder& encoder,
                          const ResponseHeader& header) {
  encoder.Encode(std::int64_t{0});
  encoder.Encode(header.request_handle);
  encoder.Encode(header.service_result.good() ? 0u : 0x80000000u);
  encoder.Encode(std::uint8_t{0});
  encoder.Encode(std::int32_t{0});
  AppendNumericNodeId(encoder, 0);
  encoder.Encode(std::uint8_t{0x00});
}

void AppendRequestHeader(Encoder& encoder,
                         const RequestHeader& header) {
  AppendNumericNodeId(encoder, 0);
  encoder.Encode(std::int64_t{0});
  encoder.Encode(header.request_handle);
  encoder.Encode(header.return_diagnostics);
  encoder.Encode(header.audit_entry_id);
  encoder.Encode(header.timeout_hint);
  AppendNumericNodeId(encoder, 0);
  encoder.Encode(std::uint8_t{0x00});
}

bool ReadResponseHeader(Decoder& decoder,
                        ResponseHeader& header) {
  std::int64_t ignored_timestamp = 0;
  std::uint32_t status_word = 0;
  std::uint8_t ignored_service_diagnostics_mask = 0;
  std::int32_t ignored_string_table_count = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(header.request_handle) ||
      !decoder.Decode(status_word) ||
      !decoder.Decode(ignored_service_diagnostics_mask) ||
      !decoder.Decode(ignored_string_table_count)) {
    return false;
  }
  header.service_result = scada::Status::FromFullCode(status_word);

  std::uint32_t additional_type_id = 0;
  std::uint8_t additional_encoding = 0;
  std::vector<char> additional_body;
  return ReadExtensionObject(decoder, additional_type_id, additional_encoding,
                             additional_body);
}

}  // namespace

std::optional<SecureConversationMessage>
DecodeSecureConversationMessage(const std::vector<char>& frame) {
  const auto frame_header = DecodeFrameHeader(frame);
  if (!frame_header || frame_header->message_size != frame.size()) {
    return std::nullopt;
  }

  SecureConversationMessage message;
  message.frame_header = *frame_header;
  Decoder decoder{std::span<const char>{frame}.subspan(8)};
  if (!decoder.Decode(message.secure_channel_id)) {
    return std::nullopt;
  }

  if (frame_header->message_type == MessageType::SecureOpen) {
    AsymmetricSecurityHeader header;
    if (!decoder.Decode(header.security_policy_uri) ||
        !decoder.Decode(header.sender_certificate) ||
        !decoder.Decode(
                        header.receiver_certificate_thumbprint)) {
      return std::nullopt;
    }
    message.asymmetric_security_header = std::move(header);
  } else {
    SymmetricSecurityHeader header;
    if (!decoder.Decode(header.token_id)) {
      return std::nullopt;
    }
    message.symmetric_security_header = header;
  }

  if (!decoder.Decode(message.sequence_header.sequence_number) ||
      !decoder.Decode(message.sequence_header.request_id)) {
    return std::nullopt;
  }

  message.body.assign(frame.begin() + static_cast<std::ptrdiff_t>(8 + decoder.offset()),
                      frame.end());
  return message;
}

std::vector<char> EncodeSecureConversationMessage(
    const SecureConversationMessage& message) {
  std::vector<char> frame = EncodeFrameHeader(
      {.message_type = message.frame_header.message_type,
       .chunk_type = message.frame_header.chunk_type,
       .message_size = 0});
  Encoder encoder{frame};
  encoder.Encode(message.secure_channel_id);

  if (message.frame_header.message_type == MessageType::SecureOpen) {
    const auto& header = *message.asymmetric_security_header;
    encoder.Encode(header.security_policy_uri);
    encoder.Encode(header.sender_certificate);
    encoder.Encode(header.receiver_certificate_thumbprint);
  } else {
    encoder.Encode(message.symmetric_security_header->token_id);
  }

  encoder.Encode(message.sequence_header.sequence_number);
  encoder.Encode(message.sequence_header.request_id);
  frame.insert(frame.end(), message.body.begin(), message.body.end());

  const auto message_size = static_cast<std::uint32_t>(frame.size());
  std::memcpy(frame.data() + 4, &message_size, sizeof(message_size));
  return frame;
}

std::optional<OpenSecureChannelRequest>
DecodeOpenSecureChannelRequestBody(const std::vector<char>& body) {
  Decoder body_decoder{body};
  std::uint32_t type_id = 0;
  std::uint8_t encoding = 0;
  std::vector<char> payload;
  if (!ReadExtensionObject(body_decoder, type_id, encoding, payload) ||
      type_id != kOpenSecureChannelRequestEncodingId || encoding != 0x01 ||
      !body_decoder.consumed()) {
    return std::nullopt;
  }

  OpenSecureChannelRequest request;
  Decoder payload_decoder{payload};
  std::uint32_t request_type = 0;
  std::uint32_t security_mode = 0;
  if (!ReadRequestHeader(payload_decoder, request.request_header) ||
      !payload_decoder.Decode(request.client_protocol_version) ||
      !payload_decoder.Decode(request_type) ||
      !payload_decoder.Decode(security_mode) ||
      !payload_decoder.Decode(request.client_nonce) ||
      !payload_decoder.Decode(request.requested_lifetime) ||
      !payload_decoder.consumed()) {
    return std::nullopt;
  }

  request.request_type =
      static_cast<SecurityTokenRequestType>(request_type);
  request.security_mode =
      static_cast<MessageSecurityMode>(security_mode);
  return request;
}

std::vector<char> EncodeOpenSecureChannelResponseBody(
    const OpenSecureChannelResponse& response) {
  std::vector<char> payload;
  Encoder payload_encoder{payload};
  AppendResponseHeader(payload_encoder, response.response_header);
  payload_encoder.Encode(response.server_protocol_version);
  payload_encoder.Encode(response.security_token.channel_id);
  payload_encoder.Encode(response.security_token.token_id);
  payload_encoder.Encode(response.security_token.created_at);
  payload_encoder.Encode(response.security_token.revised_lifetime);
  payload_encoder.Encode(response.server_nonce);

  std::vector<char> body;
  Encoder body_encoder{body};
  AppendExtensionObject(body_encoder,
                        kOpenSecureChannelResponseEncodingId, payload);
  return body;
}

std::optional<CloseSecureChannelRequest>
DecodeCloseSecureChannelRequestBody(const std::vector<char>& body) {
  Decoder body_decoder{body};
  std::uint32_t type_id = 0;
  std::uint8_t encoding = 0;
  std::vector<char> payload;
  if (!ReadExtensionObject(body_decoder, type_id, encoding, payload) ||
      type_id != kCloseSecureChannelRequestEncodingId || encoding != 0x01 ||
      !body_decoder.consumed()) {
    return std::nullopt;
  }

  CloseSecureChannelRequest request;
  Decoder payload_decoder{payload};
  if (!ReadRequestHeader(payload_decoder, request.request_header) ||
      !payload_decoder.consumed()) {
    return std::nullopt;
  }
  return request;
}

std::vector<char> EncodeOpenSecureChannelRequestBody(
    const OpenSecureChannelRequest& request) {
  std::vector<char> payload;
  Encoder payload_encoder{payload};
  AppendRequestHeader(payload_encoder, request.request_header);
  payload_encoder.Encode(request.client_protocol_version);
  payload_encoder.Encode(static_cast<std::uint32_t>(request.request_type));
  payload_encoder.Encode(static_cast<std::uint32_t>(request.security_mode));
  payload_encoder.Encode(request.client_nonce);
  payload_encoder.Encode(request.requested_lifetime);

  std::vector<char> body;
  Encoder body_encoder{body};
  AppendExtensionObject(body_encoder,
                        kOpenSecureChannelRequestEncodingId, payload);
  return body;
}

std::optional<OpenSecureChannelResponse>
DecodeOpenSecureChannelResponseBody(const std::vector<char>& body) {
  Decoder body_decoder{body};
  std::uint32_t type_id = 0;
  std::uint8_t encoding = 0;
  std::vector<char> payload;
  if (!ReadExtensionObject(body_decoder, type_id, encoding, payload) ||
      type_id != kOpenSecureChannelResponseEncodingId || encoding != 0x01 ||
      !body_decoder.consumed()) {
    return std::nullopt;
  }

  OpenSecureChannelResponse response;
  Decoder payload_decoder{payload};
  if (!ReadResponseHeader(payload_decoder, response.response_header) ||
      !payload_decoder.Decode(response.server_protocol_version) ||
      !payload_decoder.Decode(response.security_token.channel_id) ||
      !payload_decoder.Decode(response.security_token.token_id) ||
      !payload_decoder.Decode(response.security_token.created_at) ||
      !payload_decoder.Decode(response.security_token.revised_lifetime) ||
      !payload_decoder.Decode(response.server_nonce) ||
      !payload_decoder.consumed()) {
    return std::nullopt;
  }
  return response;
}

std::vector<char> EncodeCloseSecureChannelRequestBody(
    const CloseSecureChannelRequest& request) {
  std::vector<char> payload;
  Encoder payload_encoder{payload};
  AppendRequestHeader(payload_encoder, request.request_header);

  std::vector<char> body;
  Encoder body_encoder{body};
  AppendExtensionObject(body_encoder,
                        kCloseSecureChannelRequestEncodingId, payload);
  return body;
}

SecureChannel::SecureChannel(std::uint32_t channel_id)
    : channel_id_{channel_id} {}

Awaitable<SecureChannel::Result> SecureChannel::HandleFrame(
    std::vector<char> frame) {
  const auto message = DecodeSecureConversationMessage(frame);
  if (!message.has_value()) {
    co_return Result{.close_transport = true};
  }

  switch (message->frame_header.message_type) {
    case MessageType::SecureOpen: {
      const auto request = DecodeOpenSecureChannelRequestBody(message->body);
      if (!request.has_value()) {
        co_return Result{.close_transport = true};
      }

      const auto supported_security =
          request->security_mode == MessageSecurityMode::None &&
          message->asymmetric_security_header &&
          message->asymmetric_security_header->security_policy_uri ==
              kSecurityPolicyNone;

      auto response = BuildOpenResponse(
          *message, *request,
          supported_security ? scada::StatusCode::Good : scada::StatusCode::Bad);
      if (supported_security) {
        opened_ = true;
        if (request->request_type == SecurityTokenRequestType::Renew) {
          ++token_id_;
        }
      }
      co_return Result{.outbound_frame = std::move(response)};
    }

    case MessageType::SecureMessage: {
      if (!opened_ || message->secure_channel_id != channel_id_ ||
          !message->symmetric_security_header ||
          message->symmetric_security_header->token_id != token_id_) {
        co_return Result{.close_transport = true};
      }
      co_return Result{.service_payload = std::move(message->body),
                       .request_id = message->sequence_header.request_id};
    }

    case MessageType::SecureClose: {
      if (!opened_ || message->secure_channel_id != channel_id_ ||
          !message->symmetric_security_header ||
          message->symmetric_security_header->token_id != token_id_) {
        co_return Result{.close_transport = true};
      }
      const auto request = DecodeCloseSecureChannelRequestBody(message->body);
      opened_ = false;
      co_return Result{.close_transport = !request.has_value() || true};
    }

    case MessageType::Hello:
    case MessageType::Acknowledge:
    case MessageType::Error:
    case MessageType::ReverseHello:
      co_return Result{.close_transport = true};
  }

  co_return Result{.close_transport = true};
}

std::vector<char> SecureChannel::BuildServiceResponse(
    std::uint32_t request_id,
    std::vector<char> body) {
  SecureConversationMessage message{
      .frame_header =
          {.message_type = MessageType::SecureMessage,
           .chunk_type = 'F',
           .message_size = 0},
      .secure_channel_id = channel_id_,
      .symmetric_security_header =
          SymmetricSecurityHeader{.token_id = token_id_},
      .sequence_header =
          {.sequence_number = next_sequence_number_++, .request_id = request_id},
      .body = std::move(body),
  };
  return EncodeSecureConversationMessage(message);
}

std::vector<char> SecureChannel::BuildOpenResponse(
    const SecureConversationMessage& request_message,
    const OpenSecureChannelRequest& request,
    scada::Status service_result) {
  OpenSecureChannelResponse response{
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

  SecureConversationMessage message{
      .frame_header =
          {.message_type = MessageType::SecureOpen,
           .chunk_type = 'F',
           .message_size = 0},
      .secure_channel_id = channel_id_,
      .asymmetric_security_header = AsymmetricSecurityHeader{
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

}  // namespace opcua::binary
