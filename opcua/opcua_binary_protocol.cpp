#include "opcua/opcua_binary_protocol.h"
#include "opcua/opcua_binary_codec_utils.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace opcua {
namespace {

constexpr std::size_t kHeaderSize = 8;
constexpr char kFinalChunkType = 'F';

std::array<char, 3> EncodeMessageType(OpcUaBinaryMessageType message_type) {
  switch (message_type) {
    case OpcUaBinaryMessageType::Hello:
      return {'H', 'E', 'L'};
    case OpcUaBinaryMessageType::Acknowledge:
      return {'A', 'C', 'K'};
    case OpcUaBinaryMessageType::Error:
      return {'E', 'R', 'R'};
    case OpcUaBinaryMessageType::ReverseHello:
      return {'R', 'H', 'E'};
    case OpcUaBinaryMessageType::SecureOpen:
      return {'O', 'P', 'N'};
    case OpcUaBinaryMessageType::SecureMessage:
      return {'M', 'S', 'G'};
    case OpcUaBinaryMessageType::SecureClose:
      return {'C', 'L', 'O'};
  }
  return {'E', 'R', 'R'};
}

std::optional<OpcUaBinaryMessageType> DecodeMessageType(
    std::string_view message_type) {
  if (message_type == "HEL") {
    return OpcUaBinaryMessageType::Hello;
  }
  if (message_type == "ACK") {
    return OpcUaBinaryMessageType::Acknowledge;
  }
  if (message_type == "ERR") {
    return OpcUaBinaryMessageType::Error;
  }
  if (message_type == "RHE") {
    return OpcUaBinaryMessageType::ReverseHello;
  }
  if (message_type == "OPN") {
    return OpcUaBinaryMessageType::SecureOpen;
  }
  if (message_type == "MSG") {
    return OpcUaBinaryMessageType::SecureMessage;
  }
  if (message_type == "CLO") {
    return OpcUaBinaryMessageType::SecureClose;
  }
  return std::nullopt;
}

template <typename FillBody>
std::vector<char> EncodeWithHeader(OpcUaBinaryMessageType message_type,
                                   FillBody&& fill_body) {
  std::vector<char> bytes(kHeaderSize, '\0');
  fill_body(bytes);

  const auto encoded_type = EncodeMessageType(message_type);
  bytes[0] = encoded_type[0];
  bytes[1] = encoded_type[1];
  bytes[2] = encoded_type[2];
  bytes[3] = kFinalChunkType;
  const auto message_size = static_cast<std::uint32_t>(bytes.size());
  std::memcpy(bytes.data() + 4, &message_size, sizeof(message_size));
  return bytes;
}

}  // namespace

std::optional<OpcUaBinaryFrameHeader> DecodeBinaryFrameHeader(
    const std::vector<char>& bytes) {
  if (bytes.size() < kHeaderSize) {
    return std::nullopt;
  }

  const auto message_type = DecodeMessageType(
      std::string_view{bytes.data(), 3});
  if (!message_type.has_value()) {
    return std::nullopt;
  }

  OpcUaBinaryFrameHeader header{
      .message_type = *message_type,
      .chunk_type = bytes[3],
  };
  std::memcpy(&header.message_size, bytes.data() + 4,
              sizeof(header.message_size));
  if (header.message_size < kHeaderSize) {
    return std::nullopt;
  }
  return header;
}

std::vector<char> EncodeBinaryFrameHeader(
    const OpcUaBinaryFrameHeader& header) {
  std::vector<char> bytes(kHeaderSize, '\0');
  const auto message_type = EncodeMessageType(header.message_type);
  bytes[0] = message_type[0];
  bytes[1] = message_type[1];
  bytes[2] = message_type[2];
  bytes[3] = header.chunk_type;
  std::memcpy(bytes.data() + 4, &header.message_size,
              sizeof(header.message_size));
  return bytes;
}

std::vector<char> EncodeBinaryHelloMessage(
    const OpcUaBinaryHelloMessage& message) {
  return EncodeWithHeader(OpcUaBinaryMessageType::Hello,
                          [&](std::vector<char>& bytes) {
                            binary::AppendUInt32(bytes, message.protocol_version);
                            binary::AppendUInt32(bytes, message.receive_buffer_size);
                            binary::AppendUInt32(bytes, message.send_buffer_size);
                            binary::AppendUInt32(bytes, message.max_message_size);
                            binary::AppendUInt32(bytes, message.max_chunk_count);
                            binary::AppendUaString(bytes, message.endpoint_url);
                          });
}

std::optional<OpcUaBinaryHelloMessage> DecodeBinaryHelloMessage(
    const std::vector<char>& bytes) {
  const auto header = DecodeBinaryFrameHeader(bytes);
  if (!header || header->message_type != OpcUaBinaryMessageType::Hello ||
      header->message_size != bytes.size()) {
    return std::nullopt;
  }

  OpcUaBinaryHelloMessage message;
  std::size_t offset = kHeaderSize;
  if (!binary::ReadUInt32(bytes, offset, message.protocol_version) ||
      !binary::ReadUInt32(bytes, offset, message.receive_buffer_size) ||
      !binary::ReadUInt32(bytes, offset, message.send_buffer_size) ||
      !binary::ReadUInt32(bytes, offset, message.max_message_size) ||
      !binary::ReadUInt32(bytes, offset, message.max_chunk_count) ||
      !binary::ReadUaString(bytes, offset, message.endpoint_url)) {
    return std::nullopt;
  }
  if (offset != bytes.size()) {
    return std::nullopt;
  }
  return message;
}

std::vector<char> EncodeBinaryAcknowledgeMessage(
    const OpcUaBinaryAcknowledgeMessage& message) {
  return EncodeWithHeader(OpcUaBinaryMessageType::Acknowledge,
                          [&](std::vector<char>& bytes) {
                            binary::AppendUInt32(bytes, message.protocol_version);
                            binary::AppendUInt32(bytes, message.receive_buffer_size);
                            binary::AppendUInt32(bytes, message.send_buffer_size);
                            binary::AppendUInt32(bytes, message.max_message_size);
                            binary::AppendUInt32(bytes, message.max_chunk_count);
                          });
}

std::optional<OpcUaBinaryAcknowledgeMessage> DecodeBinaryAcknowledgeMessage(
    const std::vector<char>& bytes) {
  const auto header = DecodeBinaryFrameHeader(bytes);
  if (!header || header->message_type != OpcUaBinaryMessageType::Acknowledge ||
      header->message_size != bytes.size()) {
    return std::nullopt;
  }

  OpcUaBinaryAcknowledgeMessage message;
  std::size_t offset = kHeaderSize;
  if (!binary::ReadUInt32(bytes, offset, message.protocol_version) ||
      !binary::ReadUInt32(bytes, offset, message.receive_buffer_size) ||
      !binary::ReadUInt32(bytes, offset, message.send_buffer_size) ||
      !binary::ReadUInt32(bytes, offset, message.max_message_size) ||
      !binary::ReadUInt32(bytes, offset, message.max_chunk_count)) {
    return std::nullopt;
  }
  if (offset != bytes.size()) {
    return std::nullopt;
  }
  return message;
}

std::vector<char> EncodeBinaryErrorMessage(
    const OpcUaBinaryErrorMessage& message) {
  return EncodeWithHeader(OpcUaBinaryMessageType::Error,
                          [&](std::vector<char>& bytes) {
                            binary::AppendUInt32(bytes, message.error.full_code());
                            binary::AppendUaString(bytes, message.reason);
                          });
}

std::optional<OpcUaBinaryErrorMessage> DecodeBinaryErrorMessage(
    const std::vector<char>& bytes) {
  const auto header = DecodeBinaryFrameHeader(bytes);
  if (!header || header->message_type != OpcUaBinaryMessageType::Error ||
      header->message_size != bytes.size()) {
    return std::nullopt;
  }

  OpcUaBinaryErrorMessage message;
  std::size_t offset = kHeaderSize;
  std::uint32_t full_code = 0;
  if (!binary::ReadUInt32(bytes, offset, full_code) ||
      !binary::ReadUaString(bytes, offset, message.reason)) {
    return std::nullopt;
  }
  if (offset != bytes.size()) {
    return std::nullopt;
  }
  message.error = scada::Status::FromFullCode(full_code);
  return message;
}

OpcUaBinaryNegotiationResult NegotiateBinaryHello(
    const OpcUaBinaryHelloMessage& hello,
    const OpcUaBinaryTransportLimits& server_limits) {
  if (hello.receive_buffer_size == 0 || hello.send_buffer_size == 0) {
    return {.error = OpcUaBinaryErrorMessage{
                .error = scada::StatusCode::Bad,
                .reason = "Client transport buffers must be non-zero",
            }};
  }

  OpcUaBinaryAcknowledgeMessage acknowledge;
  acknowledge.protocol_version = std::min(hello.protocol_version,
                                          server_limits.protocol_version);
  acknowledge.receive_buffer_size =
      std::min(server_limits.receive_buffer_size, hello.send_buffer_size);
  acknowledge.send_buffer_size =
      std::min(server_limits.send_buffer_size, hello.receive_buffer_size);
  acknowledge.max_message_size = server_limits.max_message_size;
  acknowledge.max_chunk_count = server_limits.max_chunk_count;

  return {.acknowledge = acknowledge};
}

}  // namespace opcua
