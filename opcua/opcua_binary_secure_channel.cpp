#include "opcua/opcua_binary_secure_channel.h"

#include <cstring>

namespace opcua {
namespace {

void AppendUInt8(std::vector<char>& bytes, std::uint8_t value) {
  bytes.push_back(static_cast<char>(value));
}

void AppendUInt16(std::vector<char>& bytes, std::uint16_t value) {
  bytes.push_back(static_cast<char>(value & 0xff));
  bytes.push_back(static_cast<char>((value >> 8) & 0xff));
}

void AppendUInt32(std::vector<char>& bytes, std::uint32_t value) {
  bytes.push_back(static_cast<char>(value & 0xff));
  bytes.push_back(static_cast<char>((value >> 8) & 0xff));
  bytes.push_back(static_cast<char>((value >> 16) & 0xff));
  bytes.push_back(static_cast<char>((value >> 24) & 0xff));
}

void AppendInt32(std::vector<char>& bytes, std::int32_t value) {
  AppendUInt32(bytes, static_cast<std::uint32_t>(value));
}

void AppendInt64(std::vector<char>& bytes, std::int64_t value) {
  const auto raw = static_cast<std::uint64_t>(value);
  for (int i = 0; i < 8; ++i) {
    bytes.push_back(static_cast<char>((raw >> (8 * i)) & 0xff));
  }
}

bool ReadUInt8(const std::vector<char>& bytes,
               std::size_t& offset,
               std::uint8_t& value) {
  if (offset + 1 > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint8_t>(bytes[offset]);
  ++offset;
  return true;
}

bool ReadUInt16(const std::vector<char>& bytes,
                std::size_t& offset,
                std::uint16_t& value) {
  if (offset + 2 > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint16_t>(
      static_cast<unsigned char>(bytes[offset]) |
      (static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[offset + 1]))
       << 8));
  offset += 2;
  return true;
}

bool ReadUInt32(const std::vector<char>& bytes,
                std::size_t& offset,
                std::uint32_t& value) {
  if (offset + 4 > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint32_t>(
              static_cast<unsigned char>(bytes[offset])) |
          (static_cast<std::uint32_t>(
               static_cast<unsigned char>(bytes[offset + 1]))
           << 8) |
          (static_cast<std::uint32_t>(
               static_cast<unsigned char>(bytes[offset + 2]))
           << 16) |
          (static_cast<std::uint32_t>(
               static_cast<unsigned char>(bytes[offset + 3]))
           << 24);
  offset += 4;
  return true;
}

bool ReadInt32(const std::vector<char>& bytes,
               std::size_t& offset,
               std::int32_t& value) {
  std::uint32_t raw = 0;
  if (!ReadUInt32(bytes, offset, raw)) {
    return false;
  }
  value = static_cast<std::int32_t>(raw);
  return true;
}

bool ReadInt64(const std::vector<char>& bytes,
               std::size_t& offset,
               std::int64_t& value) {
  if (offset + 8 > bytes.size()) {
    return false;
  }
  std::uint64_t raw = 0;
  for (int i = 0; i < 8; ++i) {
    raw |= static_cast<std::uint64_t>(
               static_cast<unsigned char>(bytes[offset + i]))
           << (8 * i);
  }
  value = static_cast<std::int64_t>(raw);
  offset += 8;
  return true;
}

void AppendUaString(std::vector<char>& bytes, std::string_view value) {
  AppendInt32(bytes, static_cast<std::int32_t>(value.size()));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

bool ReadUaString(const std::vector<char>& bytes,
                  std::size_t& offset,
                  std::string& value) {
  std::int32_t length = 0;
  if (!ReadInt32(bytes, offset, length)) {
    return false;
  }
  if (length < 0) {
    value.clear();
    return true;
  }
  if (offset + static_cast<std::size_t>(length) > bytes.size()) {
    return false;
  }
  value.assign(bytes.data() + offset, static_cast<std::size_t>(length));
  offset += static_cast<std::size_t>(length);
  return true;
}

void AppendByteString(std::vector<char>& bytes,
                      const scada::ByteString& value) {
  AppendInt32(bytes, static_cast<std::int32_t>(value.size()));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

bool ReadByteString(const std::vector<char>& bytes,
                    std::size_t& offset,
                    scada::ByteString& value) {
  std::int32_t length = 0;
  if (!ReadInt32(bytes, offset, length)) {
    return false;
  }
  if (length < 0) {
    value.clear();
    return true;
  }
  if (offset + static_cast<std::size_t>(length) > bytes.size()) {
    return false;
  }
  value.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
               bytes.begin() + static_cast<std::ptrdiff_t>(offset + length));
  offset += static_cast<std::size_t>(length);
  return true;
}

void AppendNumericNodeId(std::vector<char>& bytes, std::uint32_t id) {
  if (id == 0) {
    AppendUInt8(bytes, 0x00);
    return;
  }
  if (id <= 0xffff) {
    AppendUInt8(bytes, 0x01);
    AppendUInt8(bytes, 0);
    AppendUInt16(bytes, static_cast<std::uint16_t>(id));
    return;
  }
  AppendUInt8(bytes, 0x02);
  AppendUInt16(bytes, 0);
  AppendUInt32(bytes, id);
}

bool ReadNumericNodeId(const std::vector<char>& bytes,
                       std::size_t& offset,
                       std::uint32_t& id) {
  std::uint8_t encoding = 0;
  if (!ReadUInt8(bytes, offset, encoding)) {
    return false;
  }
  if (encoding == 0x00) {
    id = 0;
    return true;
  }
  if (encoding == 0x01) {
    std::uint8_t ns = 0;
    std::uint16_t short_id = 0;
    if (!ReadUInt8(bytes, offset, ns) || !ReadUInt16(bytes, offset, short_id) ||
        ns != 0) {
      return false;
    }
    id = short_id;
    return true;
  }
  if (encoding == 0x02) {
    std::uint16_t ns = 0;
    if (!ReadUInt16(bytes, offset, ns) || ns != 0 ||
        !ReadUInt32(bytes, offset, id)) {
      return false;
    }
    return true;
  }
  return false;
}

void AppendExtensionObject(std::vector<char>& bytes,
                           std::uint32_t type_id,
                           const std::vector<char>& body) {
  AppendNumericNodeId(bytes, type_id);
  AppendUInt8(bytes, 0x01);
  AppendInt32(bytes, static_cast<std::int32_t>(body.size()));
  bytes.insert(bytes.end(), body.begin(), body.end());
}

bool ReadExtensionObject(const std::vector<char>& bytes,
                         std::size_t& offset,
                         std::uint32_t& type_id,
                         std::uint8_t& encoding,
                         std::vector<char>& body) {
  if (!ReadNumericNodeId(bytes, offset, type_id) ||
      !ReadUInt8(bytes, offset, encoding)) {
    return false;
  }
  if (encoding == 0x00) {
    body.clear();
    return true;
  }
  std::int32_t length = 0;
  if (!ReadInt32(bytes, offset, length) || length < 0 ||
      offset + static_cast<std::size_t>(length) > bytes.size()) {
    return false;
  }
  body.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
              bytes.begin() +
                  static_cast<std::ptrdiff_t>(offset + static_cast<std::size_t>(length)));
  offset += static_cast<std::size_t>(length);
  return true;
}

bool ReadRequestHeader(const std::vector<char>& bytes,
                       std::size_t& offset,
                       OpcUaBinaryRequestHeader& header) {
  std::uint32_t ignored_node_id = 0;
  std::int64_t ignored_timestamp = 0;
  if (!ReadNumericNodeId(bytes, offset, ignored_node_id) ||
      !ReadInt64(bytes, offset, ignored_timestamp) ||
      !ReadUInt32(bytes, offset, header.request_handle) ||
      !ReadUInt32(bytes, offset, header.return_diagnostics) ||
      !ReadUaString(bytes, offset, header.audit_entry_id) ||
      !ReadUInt32(bytes, offset, header.timeout_hint)) {
    return false;
  }

  std::uint32_t additional_type_id = 0;
  std::uint8_t additional_encoding = 0;
  std::vector<char> additional_body;
  return ReadExtensionObject(bytes, offset, additional_type_id,
                             additional_encoding, additional_body);
}

void AppendResponseHeader(std::vector<char>& bytes,
                          const OpcUaBinaryResponseHeader& header) {
  AppendInt64(bytes, 0);
  AppendUInt32(bytes, header.request_handle);
  AppendUInt32(bytes, header.service_result.good() ? 0u : 0x80000000u);
  AppendUInt8(bytes, 0);
  AppendInt32(bytes, 0);
  AppendNumericNodeId(bytes, 0);
  AppendUInt8(bytes, 0x00);
}

void AppendRequestHeader(std::vector<char>& bytes,
                         const OpcUaBinaryRequestHeader& header) {
  AppendNumericNodeId(bytes, 0);
  AppendInt64(bytes, 0);
  AppendUInt32(bytes, header.request_handle);
  AppendUInt32(bytes, header.return_diagnostics);
  AppendUaString(bytes, header.audit_entry_id);
  AppendUInt32(bytes, header.timeout_hint);
  AppendNumericNodeId(bytes, 0);
  AppendUInt8(bytes, 0x00);
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
  std::size_t offset = 8;
  if (!ReadUInt32(frame, offset, message.secure_channel_id)) {
    return std::nullopt;
  }

  if (frame_header->message_type == OpcUaBinaryMessageType::SecureOpen) {
    OpcUaBinaryAsymmetricSecurityHeader header;
    if (!ReadUaString(frame, offset, header.security_policy_uri) ||
        !ReadByteString(frame, offset, header.sender_certificate) ||
        !ReadByteString(frame, offset,
                        header.receiver_certificate_thumbprint)) {
      return std::nullopt;
    }
    message.asymmetric_security_header = std::move(header);
  } else {
    OpcUaBinarySymmetricSecurityHeader header;
    if (!ReadUInt32(frame, offset, header.token_id)) {
      return std::nullopt;
    }
    message.symmetric_security_header = header;
  }

  if (!ReadUInt32(frame, offset, message.sequence_header.sequence_number) ||
      !ReadUInt32(frame, offset, message.sequence_header.request_id)) {
    return std::nullopt;
  }

  message.body.assign(frame.begin() + static_cast<std::ptrdiff_t>(offset),
                      frame.end());
  return message;
}

std::vector<char> EncodeSecureConversationMessage(
    const OpcUaBinarySecureConversationMessage& message) {
  std::vector<char> frame = EncodeBinaryFrameHeader(
      {.message_type = message.frame_header.message_type,
       .chunk_type = message.frame_header.chunk_type,
       .message_size = 0});
  AppendUInt32(frame, message.secure_channel_id);

  if (message.frame_header.message_type == OpcUaBinaryMessageType::SecureOpen) {
    const auto& header = *message.asymmetric_security_header;
    AppendUaString(frame, header.security_policy_uri);
    AppendByteString(frame, header.sender_certificate);
    AppendByteString(frame, header.receiver_certificate_thumbprint);
  } else {
    AppendUInt32(frame, message.symmetric_security_header->token_id);
  }

  AppendUInt32(frame, message.sequence_header.sequence_number);
  AppendUInt32(frame, message.sequence_header.request_id);
  frame.insert(frame.end(), message.body.begin(), message.body.end());

  const auto message_size = static_cast<std::uint32_t>(frame.size());
  std::memcpy(frame.data() + 4, &message_size, sizeof(message_size));
  return frame;
}

std::optional<OpcUaBinaryOpenSecureChannelRequest>
DecodeOpenSecureChannelRequestBody(const std::vector<char>& body) {
  std::size_t offset = 0;
  std::uint32_t type_id = 0;
  std::uint8_t encoding = 0;
  std::vector<char> payload;
  if (!ReadExtensionObject(body, offset, type_id, encoding, payload) ||
      type_id != kOpenSecureChannelRequestBinaryEncodingId || encoding != 0x01 ||
      offset != body.size()) {
    return std::nullopt;
  }

  OpcUaBinaryOpenSecureChannelRequest request;
  std::size_t payload_offset = 0;
  std::uint32_t request_type = 0;
  std::uint32_t security_mode = 0;
  if (!ReadRequestHeader(payload, payload_offset, request.request_header) ||
      !ReadUInt32(payload, payload_offset, request.client_protocol_version) ||
      !ReadUInt32(payload, payload_offset, request_type) ||
      !ReadUInt32(payload, payload_offset, security_mode) ||
      !ReadByteString(payload, payload_offset, request.client_nonce) ||
      !ReadUInt32(payload, payload_offset, request.requested_lifetime) ||
      payload_offset != payload.size()) {
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
  AppendResponseHeader(payload, response.response_header);
  AppendUInt32(payload, response.server_protocol_version);
  AppendUInt32(payload, response.security_token.channel_id);
  AppendUInt32(payload, response.security_token.token_id);
  AppendInt64(payload, response.security_token.created_at);
  AppendUInt32(payload, response.security_token.revised_lifetime);
  AppendByteString(payload, response.server_nonce);

  std::vector<char> body;
  AppendExtensionObject(body, kOpenSecureChannelResponseBinaryEncodingId,
                        payload);
  return body;
}

std::optional<OpcUaBinaryCloseSecureChannelRequest>
DecodeCloseSecureChannelRequestBody(const std::vector<char>& body) {
  std::size_t offset = 0;
  std::uint32_t type_id = 0;
  std::uint8_t encoding = 0;
  std::vector<char> payload;
  if (!ReadExtensionObject(body, offset, type_id, encoding, payload) ||
      type_id != kCloseSecureChannelRequestBinaryEncodingId || encoding != 0x01 ||
      offset != body.size()) {
    return std::nullopt;
  }

  OpcUaBinaryCloseSecureChannelRequest request;
  std::size_t payload_offset = 0;
  if (!ReadRequestHeader(payload, payload_offset, request.request_header) ||
      payload_offset != payload.size()) {
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
