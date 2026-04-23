#pragma once

#include "base/awaitable.h"
#include "opcua/binary/opcua_binary_protocol.h"
#include "scada/basic_types.h"
#include "scada/status.h"

#include <optional>
#include <string>
#include <vector>

namespace opcua {

constexpr std::uint32_t kOpenSecureChannelRequestBinaryEncodingId = 446;
constexpr std::uint32_t kOpenSecureChannelResponseBinaryEncodingId = 449;
constexpr std::uint32_t kCloseSecureChannelRequestBinaryEncodingId = 452;
constexpr std::uint32_t kCloseSecureChannelResponseBinaryEncodingId = 455;

constexpr std::string_view kSecurityPolicyNone =
    "http://opcfoundation.org/UA/SecurityPolicy#None";

enum class OpcUaBinarySecurityTokenRequestType : std::uint32_t {
  Issue = 0,
  Renew = 1,
};

enum class OpcUaBinaryMessageSecurityMode : std::uint32_t {
  Invalid = 0,
  None = 1,
  Sign = 2,
  SignAndEncrypt = 3,
};

struct OpcUaBinaryRequestHeader {
  std::uint32_t request_handle = 0;
  std::uint32_t return_diagnostics = 0;
  std::string audit_entry_id;
  std::uint32_t timeout_hint = 0;
};

struct OpcUaBinaryResponseHeader {
  std::uint32_t request_handle = 0;
  scada::Status service_result = scada::StatusCode::Good;
};

struct OpcUaBinaryChannelSecurityToken {
  std::uint32_t channel_id = 0;
  std::uint32_t token_id = 0;
  std::int64_t created_at = 0;
  std::uint32_t revised_lifetime = 0;
};

struct OpcUaBinaryAsymmetricSecurityHeader {
  std::string security_policy_uri;
  scada::ByteString sender_certificate;
  scada::ByteString receiver_certificate_thumbprint;
};

struct OpcUaBinarySymmetricSecurityHeader {
  std::uint32_t token_id = 0;
};

struct OpcUaBinarySequenceHeader {
  std::uint32_t sequence_number = 0;
  std::uint32_t request_id = 0;
};

struct OpcUaBinarySecureConversationMessage {
  OpcUaBinaryFrameHeader frame_header;
  std::uint32_t secure_channel_id = 0;
  std::optional<OpcUaBinaryAsymmetricSecurityHeader> asymmetric_security_header;
  std::optional<OpcUaBinarySymmetricSecurityHeader> symmetric_security_header;
  OpcUaBinarySequenceHeader sequence_header;
  std::vector<char> body;
};

struct OpcUaBinaryOpenSecureChannelRequest {
  OpcUaBinaryRequestHeader request_header;
  std::uint32_t client_protocol_version = 0;
  OpcUaBinarySecurityTokenRequestType request_type =
      OpcUaBinarySecurityTokenRequestType::Issue;
  OpcUaBinaryMessageSecurityMode security_mode =
      OpcUaBinaryMessageSecurityMode::None;
  scada::ByteString client_nonce;
  std::uint32_t requested_lifetime = 0;
};

struct OpcUaBinaryOpenSecureChannelResponse {
  OpcUaBinaryResponseHeader response_header;
  std::uint32_t server_protocol_version = 0;
  OpcUaBinaryChannelSecurityToken security_token;
  scada::ByteString server_nonce;
};

struct OpcUaBinaryCloseSecureChannelRequest {
  OpcUaBinaryRequestHeader request_header;
};

[[nodiscard]] std::optional<OpcUaBinarySecureConversationMessage>
DecodeSecureConversationMessage(const std::vector<char>& frame);
[[nodiscard]] std::vector<char> EncodeSecureConversationMessage(
    const OpcUaBinarySecureConversationMessage& message);

[[nodiscard]] std::optional<OpcUaBinaryOpenSecureChannelRequest>
DecodeOpenSecureChannelRequestBody(const std::vector<char>& body);
[[nodiscard]] std::vector<char> EncodeOpenSecureChannelResponseBody(
    const OpcUaBinaryOpenSecureChannelResponse& response);
[[nodiscard]] std::optional<OpcUaBinaryCloseSecureChannelRequest>
DecodeCloseSecureChannelRequestBody(const std::vector<char>& body);

class OpcUaBinarySecureChannel {
 public:
  struct Result {
    std::optional<std::vector<char>> outbound_frame;
    std::optional<std::vector<char>> service_payload;
    std::optional<std::uint32_t> request_id;
    bool close_transport = false;
  };

  explicit OpcUaBinarySecureChannel(std::uint32_t channel_id = 1);

  [[nodiscard]] Awaitable<Result> HandleFrame(std::vector<char> frame);
  [[nodiscard]] std::vector<char> BuildServiceResponse(
      std::uint32_t request_id,
      std::vector<char> body);

  [[nodiscard]] bool opened() const { return opened_; }
  [[nodiscard]] std::uint32_t channel_id() const { return channel_id_; }
  [[nodiscard]] std::uint32_t token_id() const { return token_id_; }

 private:
  [[nodiscard]] std::vector<char> BuildOpenResponse(
      const OpcUaBinarySecureConversationMessage& request_message,
      const OpcUaBinaryOpenSecureChannelRequest& request,
      scada::Status service_result);

  std::uint32_t channel_id_;
  std::uint32_t token_id_ = 1;
  std::uint32_t next_sequence_number_ = 1;
  bool opened_ = false;
};

}  // namespace opcua
