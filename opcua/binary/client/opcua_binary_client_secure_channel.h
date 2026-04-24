#pragma once

#include "base/awaitable.h"
#include "opcua/binary/client/opcua_binary_client_transport.h"
#include "opcua/binary/client/opcua_binary_crypto.h"
#include "opcua/binary/opcua_binary_secure_channel.h"
#include "scada/basic_types.h"
#include "scada/status.h"
#include "scada/status_or.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace opcua {

// Client-side counterpart of the server's OpcUaBinarySecureChannel. Drives
// the OpenSecureChannel handshake, attaches symmetric headers to outgoing
// service requests, strips them from incoming responses, and issues a
// CloseSecureChannel when the channel is no longer needed.
//
// Two operating modes:
//   - SecurityPolicy=None, SecurityMode=None (default): no crypto transforms.
//   - SecurityPolicy=Basic256Sha256, SecurityMode=SignAndEncrypt:
//     asymmetric OPN uses RSA-OAEP-SHA1 + RSA-PKCS1-SHA256; symmetric MSG
//     uses AES-256-CBC + HMAC-SHA256 with keys derived from the nonce
//     exchange (OPC UA Part 6 §6.7). Sign-only is not implemented since
//     the outbound client callers always want end-to-end confidentiality.
class OpcUaBinaryClientSecureChannel {
 public:
  // Security configuration for a single channel. Empty for None mode.
  struct Security {
    std::string security_policy_uri = std::string{kSecurityPolicyNone};
    OpcUaBinaryMessageSecurityMode security_mode =
        OpcUaBinaryMessageSecurityMode::None;
    // All three are only read when policy != None.
    crypto::Certificate client_certificate;
    crypto::PrivateKey client_private_key;
    crypto::Certificate server_certificate;
    // Basic256Sha256 only. Defaults to a CSPRNG-backed 32-byte nonce; tests
    // may inject a deterministic generator.
    std::function<scada::StatusOr<scada::ByteString>()> client_nonce_generator;
  };

  explicit OpcUaBinaryClientSecureChannel(OpcUaBinaryClientTransport& transport);
  OpcUaBinaryClientSecureChannel(OpcUaBinaryClientTransport& transport,
                                  Security security);

  OpcUaBinaryClientSecureChannel(const OpcUaBinaryClientSecureChannel&) = delete;
  OpcUaBinaryClientSecureChannel& operator=(
      const OpcUaBinaryClientSecureChannel&) = delete;

  // Sends the OpenSecureChannel request, waits for the response, and stores
  // the negotiated channel_id / token_id. For Basic256Sha256 SignAndEncrypt
  // also derives the symmetric keys used by subsequent service traffic.
  [[nodiscard]] Awaitable<scada::Status> Open(
      std::uint32_t requested_lifetime_ms = 60000);

  // Renews the current SecureChannel token with an OpenSecureChannel request
  // whose request_type is Renew. The server returns a fresh token_id and
  // revised lifetime while preserving the logical channel.
  [[nodiscard]] Awaitable<scada::Status> Renew(
      std::uint32_t requested_lifetime_ms = 60000);

  // Wraps `body` into a symmetric SecureMessage frame and writes it to the
  // transport. `request_id` uniquely identifies the in-flight request for
  // the client channel's correlation table.
  [[nodiscard]] Awaitable<scada::Status> SendServiceRequest(
      std::uint32_t request_id,
      const std::vector<char>& body);

  struct ServiceResponse {
    std::uint32_t request_id = 0;
    std::vector<char> body;
  };
  [[nodiscard]] Awaitable<scada::StatusOr<ServiceResponse>>
  ReadServiceResponse();

  // Sends a CloseSecureChannel request (best-effort; does not wait for a
  // response because the server closes the transport immediately after).
  [[nodiscard]] Awaitable<scada::Status> Close();

  [[nodiscard]] bool opened() const { return opened_; }
  [[nodiscard]] std::uint32_t channel_id() const { return channel_id_; }
  [[nodiscard]] std::uint32_t token_id() const { return token_id_; }
  [[nodiscard]] std::uint32_t revised_lifetime_ms() const {
    return revised_lifetime_ms_;
  }

  [[nodiscard]] std::uint32_t NextRequestId();

 private:
  [[nodiscard]] bool UsesBasic256Sha256() const;
  [[nodiscard]] bool UsesSignAndEncrypt() const;
  [[nodiscard]] scada::StatusOr<scada::ByteString> GenerateClientNonce();
  [[nodiscard]] bool ShouldRenew() const;
  void ArmRenewalTimer(std::uint32_t revised_lifetime_ms);
  [[nodiscard]] Awaitable<scada::Status> RenewIfNeeded();
  [[nodiscard]] Awaitable<scada::Status> OpenSecureChannel(
      OpcUaBinarySecurityTokenRequestType request_type,
      std::uint32_t requested_lifetime_ms);

  // Build a plaintext OPN frame (no signing, no encryption). Used for the
  // None path and as the pre-sign plaintext for Basic256Sha256.
  [[nodiscard]] std::vector<char> BuildPlaintextOpenFrame(
      std::uint32_t request_id,
      std::uint32_t request_handle,
      OpcUaBinarySecurityTokenRequestType request_type,
      std::uint32_t secure_channel_id,
      const scada::ByteString& client_nonce,
      std::uint32_t requested_lifetime_ms);

  // Returns the final bytes that go on the wire for an OPN request under
  // Basic256Sha256 SignAndEncrypt: assemble plaintext, pad, sign, encrypt.
  [[nodiscard]] scada::StatusOr<std::vector<char>>
  BuildAsymmetricBasic256Sha256OpenFrame(std::uint32_t request_id,
                                          std::uint32_t request_handle,
                                          OpcUaBinarySecurityTokenRequestType request_type,
                                          std::uint32_t secure_channel_id,
                                          const scada::ByteString& client_nonce,
                                          std::uint32_t requested_lifetime_ms);

  // Parses the bytes of an OPN response under Basic256Sha256
  // SignAndEncrypt: split on the asymmetric header, RSA-OAEP decrypt,
  // verify RSA-PKCS1-SHA256 signature, extract sequence header + body.
  struct AsymmetricDecodedResponse {
    OpcUaBinaryAsymmetricSecurityHeader security_header;
    OpcUaBinarySequenceHeader sequence_header;
    std::vector<char> body;
  };
  [[nodiscard]] scada::StatusOr<AsymmetricDecodedResponse>
  DecodeAsymmetricBasic256Sha256OpenFrame(const std::vector<char>& frame);

  // Symmetric SignAndEncrypt (MSG / CLO) framing helpers.
  [[nodiscard]] scada::StatusOr<std::vector<char>>
  BuildSymmetricBasic256Sha256Frame(OpcUaBinaryMessageType type,
                                     std::uint32_t request_id,
                                     const std::vector<char>& body);
  [[nodiscard]] scada::StatusOr<ServiceResponse>
  DecodeSymmetricBasic256Sha256Frame(const std::vector<char>& frame);

  OpcUaBinaryClientTransport& transport_;
  Security security_;

  bool opened_ = false;
  std::uint32_t channel_id_ = 0;
  std::uint32_t token_id_ = 0;
  std::uint32_t revised_lifetime_ms_ = 0;
  std::chrono::steady_clock::time_point renew_at_{};
  std::uint32_t next_sequence_number_ = 1;
  std::uint32_t next_request_id_ = 1;

  // Derived symmetric keys (Basic256Sha256 only). client_* are used to
  // encrypt/sign outbound messages; server_* are used to verify/decrypt
  // inbound messages.
  crypto::DerivedKeys client_keys_;
  crypto::DerivedKeys server_keys_;
  scada::ByteString client_nonce_;
};

}  // namespace opcua
