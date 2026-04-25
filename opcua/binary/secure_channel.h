#pragma once

#include "base/awaitable.h"
#include "opcua/binary/protocol.h"
#include "scada/basic_types.h"
#include "scada/status.h"

#include <optional>
#include <string>
#include <vector>

namespace opcua::binary {

constexpr std::uint32_t kOpenSecureChannelRequestEncodingId = 446;
constexpr std::uint32_t kOpenSecureChannelResponseEncodingId = 449;
constexpr std::uint32_t kCloseSecureChannelRequestEncodingId = 452;
constexpr std::uint32_t kCloseSecureChannelResponseEncodingId = 455;

constexpr std::string_view kSecurityPolicyNone =
    "http://opcfoundation.org/UA/SecurityPolicy#None";

enum class SecurityTokenRequestType : std::uint32_t {
  Issue = 0,
  Renew = 1,
};

enum class MessageSecurityMode : std::uint32_t {
  Invalid = 0,
  None = 1,
  Sign = 2,
  SignAndEncrypt = 3,
};

struct RequestHeader {
  std::uint32_t request_handle = 0;
  std::uint32_t return_diagnostics = 0;
  std::string audit_entry_id;
  std::uint32_t timeout_hint = 0;
};

struct ResponseHeader {
  std::uint32_t request_handle = 0;
  scada::Status service_result = scada::StatusCode::Good;
};

struct ChannelSecurityToken {
  std::uint32_t channel_id = 0;
  std::uint32_t token_id = 0;
  std::int64_t created_at = 0;
  std::uint32_t revised_lifetime = 0;
};

struct AsymmetricSecurityHeader {
  std::string security_policy_uri;
  scada::ByteString sender_certificate;
  scada::ByteString receiver_certificate_thumbprint;
};

struct SymmetricSecurityHeader {
  std::uint32_t token_id = 0;
};

struct SequenceHeader {
  std::uint32_t sequence_number = 0;
  std::uint32_t request_id = 0;
};

struct SecureConversationMessage {
  FrameHeader frame_header;
  std::uint32_t secure_channel_id = 0;
  std::optional<AsymmetricSecurityHeader> asymmetric_security_header;
  std::optional<SymmetricSecurityHeader> symmetric_security_header;
  SequenceHeader sequence_header;
  std::vector<char> body;
};

struct OpenSecureChannelRequest {
  RequestHeader request_header;
  std::uint32_t client_protocol_version = 0;
  SecurityTokenRequestType request_type =
      SecurityTokenRequestType::Issue;
  MessageSecurityMode security_mode =
      MessageSecurityMode::None;
  scada::ByteString client_nonce;
  std::uint32_t requested_lifetime = 0;
};

struct OpenSecureChannelResponse {
  ResponseHeader response_header;
  std::uint32_t server_protocol_version = 0;
  ChannelSecurityToken security_token;
  scada::ByteString server_nonce;
};

struct CloseSecureChannelRequest {
  RequestHeader request_header;
};

[[nodiscard]] std::optional<SecureConversationMessage>
DecodeSecureConversationMessage(const std::vector<char>& frame);
[[nodiscard]] std::vector<char> EncodeSecureConversationMessage(
    const SecureConversationMessage& message);

[[nodiscard]] std::optional<OpenSecureChannelRequest>
DecodeOpenSecureChannelRequestBody(const std::vector<char>& body);
[[nodiscard]] std::vector<char> EncodeOpenSecureChannelResponseBody(
    const OpenSecureChannelResponse& response);
[[nodiscard]] std::optional<CloseSecureChannelRequest>
DecodeCloseSecureChannelRequestBody(const std::vector<char>& body);

// Client-side inverses: these encode/decode the complementary direction of
// the OPC UA Part 6 SecureChannel request/response pairs, so the new UA
// Binary client under common/opcua/binary/ can talk to the existing
// server-side SecureChannel using the same bytes on the wire.
[[nodiscard]] std::vector<char> EncodeOpenSecureChannelRequestBody(
    const OpenSecureChannelRequest& request);
[[nodiscard]] std::optional<OpenSecureChannelResponse>
DecodeOpenSecureChannelResponseBody(const std::vector<char>& body);
[[nodiscard]] std::vector<char> EncodeCloseSecureChannelRequestBody(
    const CloseSecureChannelRequest& request);

class SecureChannel {
 public:
  struct Result {
    std::optional<std::vector<char>> outbound_frame;
    std::optional<std::vector<char>> service_payload;
    std::optional<std::uint32_t> request_id;
    bool close_transport = false;
  };

  explicit SecureChannel(std::uint32_t channel_id = 1);

  [[nodiscard]] Awaitable<Result> HandleFrame(std::vector<char> frame);
  [[nodiscard]] std::vector<char> BuildServiceResponse(
      std::uint32_t request_id,
      std::vector<char> body);

  [[nodiscard]] bool opened() const { return opened_; }
  [[nodiscard]] std::uint32_t channel_id() const { return channel_id_; }
  [[nodiscard]] std::uint32_t token_id() const { return token_id_; }

 private:
  [[nodiscard]] std::vector<char> BuildOpenResponse(
      const SecureConversationMessage& request_message,
      const OpenSecureChannelRequest& request,
      scada::Status service_result);

  std::uint32_t channel_id_;
  std::uint32_t token_id_ = 1;
  std::uint32_t next_sequence_number_ = 1;
  bool opened_ = false;
};

}  // namespace opcua::binary
