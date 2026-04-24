#pragma once

#include "base/awaitable.h"
#include "opcua/binary/client/opcua_binary_client_transport.h"
#include "opcua/binary/opcua_binary_secure_channel.h"
#include "scada/status.h"
#include "scada/status_or.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace opcua {

// Client-side counterpart of the server's OpcUaBinarySecureChannel. Drives
// the OpenSecureChannel handshake, attaches symmetric headers to outgoing
// service requests, strips them from incoming responses, and issues a
// CloseSecureChannel when the channel is no longer needed.
//
// This initial revision supports only SecurityPolicy=None /
// SecurityMode=None; no crypto transforms are applied. The class is scoped
// so that Basic256Sha256 sign-and-encrypt can be added as a follow-up by
// plugging a crypto helper in without changing the public surface.
class OpcUaBinaryClientSecureChannel {
 public:
  explicit OpcUaBinaryClientSecureChannel(OpcUaBinaryClientTransport& transport);

  // Sends the OpenSecureChannel request, waits for the response, and stores
  // the negotiated channel_id / token_id. The transport's Hello/Acknowledge
  // handshake is assumed to have completed beforehand.
  [[nodiscard]] Awaitable<scada::Status> Open(
      std::uint32_t requested_lifetime_ms = 60000);

  // Wraps `body` into a symmetric SecureMessage frame and writes it to the
  // transport. `request_id` uniquely identifies the in-flight request for
  // the client channel's correlation table.
  [[nodiscard]] Awaitable<scada::Status> SendServiceRequest(
      std::uint32_t request_id,
      const std::vector<char>& body);

  // Reads the next SecureMessage frame from the transport and returns
  // (request_id, service_payload). Returns a bad status if the frame isn't a
  // SecureMessage for this channel/token, is truncated, or the transport
  // has closed.
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

  // Returns a monotonically increasing request id for the caller to pass
  // back into SendServiceRequest. Starts at 1.
  [[nodiscard]] std::uint32_t NextRequestId();

 private:
  OpcUaBinaryClientTransport& transport_;

  bool opened_ = false;
  std::uint32_t channel_id_ = 0;
  std::uint32_t token_id_ = 0;
  std::uint32_t next_sequence_number_ = 1;
  std::uint32_t next_request_id_ = 1;
};

}  // namespace opcua
