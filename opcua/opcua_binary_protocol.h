#pragma once

#include "scada/status.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace opcua {

enum class OpcUaBinaryMessageType {
  Hello,
  Acknowledge,
  Error,
  ReverseHello,
  SecureOpen,
  SecureMessage,
  SecureClose,
};

struct OpcUaBinaryFrameHeader {
  OpcUaBinaryMessageType message_type = OpcUaBinaryMessageType::Hello;
  char chunk_type = 'F';
  std::uint32_t message_size = 0;
};

struct OpcUaBinaryHelloMessage {
  std::uint32_t protocol_version = 0;
  std::uint32_t receive_buffer_size = 0;
  std::uint32_t send_buffer_size = 0;
  std::uint32_t max_message_size = 0;
  std::uint32_t max_chunk_count = 0;
  std::string endpoint_url;
};

struct OpcUaBinaryAcknowledgeMessage {
  std::uint32_t protocol_version = 0;
  std::uint32_t receive_buffer_size = 0;
  std::uint32_t send_buffer_size = 0;
  std::uint32_t max_message_size = 0;
  std::uint32_t max_chunk_count = 0;
};

struct OpcUaBinaryErrorMessage {
  scada::Status error = scada::StatusCode::Bad;
  std::string reason;
};

struct OpcUaBinaryTransportLimits {
  std::uint32_t protocol_version = 0;
  std::uint32_t receive_buffer_size = 65535;
  std::uint32_t send_buffer_size = 65535;
  std::uint32_t max_message_size = 0;
  std::uint32_t max_chunk_count = 0;
};

struct OpcUaBinaryNegotiationResult {
  std::optional<OpcUaBinaryAcknowledgeMessage> acknowledge;
  std::optional<OpcUaBinaryErrorMessage> error;
};

[[nodiscard]] std::optional<OpcUaBinaryFrameHeader> DecodeBinaryFrameHeader(
    const std::vector<char>& bytes);
[[nodiscard]] std::vector<char> EncodeBinaryFrameHeader(
    const OpcUaBinaryFrameHeader& header);

[[nodiscard]] std::vector<char> EncodeBinaryHelloMessage(
    const OpcUaBinaryHelloMessage& message);
[[nodiscard]] std::optional<OpcUaBinaryHelloMessage> DecodeBinaryHelloMessage(
    const std::vector<char>& bytes);

[[nodiscard]] std::vector<char> EncodeBinaryAcknowledgeMessage(
    const OpcUaBinaryAcknowledgeMessage& message);
[[nodiscard]] std::optional<OpcUaBinaryAcknowledgeMessage>
DecodeBinaryAcknowledgeMessage(const std::vector<char>& bytes);

[[nodiscard]] std::vector<char> EncodeBinaryErrorMessage(
    const OpcUaBinaryErrorMessage& message);
[[nodiscard]] std::optional<OpcUaBinaryErrorMessage> DecodeBinaryErrorMessage(
    const std::vector<char>& bytes);

// OPC UA Part 6 7.1.2.3/7.1.2.4: the server returns the protocol version it
// will use, constrains send/receive buffers to what the peer can handle, and
// otherwise reports transport-level errors before any SecureChannel messages.
[[nodiscard]] OpcUaBinaryNegotiationResult NegotiateBinaryHello(
    const OpcUaBinaryHelloMessage& hello,
    const OpcUaBinaryTransportLimits& server_limits);

}  // namespace opcua
