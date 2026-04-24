#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "opcua/binary/client/opcua_binary_client_secure_channel.h"
#include "opcua/binary/client/opcua_binary_client_transport.h"
#include "opcua/binary/opcua_binary_service_codec.h"
#include "opcua/opcua_message.h"
#include "scada/status.h"
#include "scada/status_or.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace opcua {

// Request-response correlation layer on top of OpcUaBinaryClientSecureChannel.
//
// For each typed OPC UA service request the caller provides a request_handle
// and an OpcUaRequestBody variant. The channel:
//   1. Serializes the request via EncodeOpcUaBinaryServiceRequest.
//   2. Wraps it in a SecureMessage and writes it to the transport.
//   3. Reads the next frame, decodes it via DecodeOpcUaBinaryServiceResponse,
//      and hands the typed response back.
//
// This simplified synchronous round-trip fits the client's use case: all
// outbound calls go through this one channel and are sequenced. A later
// revision can add pipelining (an internal request-id -> completion map and
// a background reader coroutine) behind the same public API without
// changing call sites.
class OpcUaBinaryClientChannel {
 public:
  struct Context {
    OpcUaBinaryClientTransport& transport;
    OpcUaBinaryClientSecureChannel& secure_channel;
    // Authentication token from a successful CreateSession; empty node id
    // before session activation.
    scada::NodeId authentication_token;
  };

  explicit OpcUaBinaryClientChannel(Context context);

  // Allocates a fresh request handle (monotonically increasing).
  [[nodiscard]] std::uint32_t NextRequestHandle();

  // Sets the session authentication token to attach to subsequent requests.
  // Called after a successful ActivateSession.
  void set_authentication_token(scada::NodeId token);
  [[nodiscard]] const scada::NodeId& authentication_token() const {
    return authentication_token_;
  }

  // Sends `request` and awaits the matching response. The returned body's
  // concrete type depends on the request; callers typically `std::get` or
  // `std::visit` on the variant.
  [[nodiscard]] Awaitable<scada::StatusOr<OpcUaResponseBody>> Call(
      std::uint32_t request_handle,
      OpcUaRequestBody request);

  // Lower-level split send/receive API for callers that keep multiple
  // requests outstanding (Publish). `Receive` buffers unrelated responses
  // for later matching by request_id.
  [[nodiscard]] Awaitable<scada::StatusOr<std::uint32_t>> Send(
      std::uint32_t request_handle,
      OpcUaRequestBody request);
  [[nodiscard]] Awaitable<scada::StatusOr<OpcUaResponseBody>> Receive(
      std::uint32_t request_id,
      std::uint32_t request_handle);

 private:
  struct BufferedResponse {
    std::uint32_t request_handle = 0;
    OpcUaResponseBody body;
  };

  OpcUaBinaryClientTransport& transport_;
  OpcUaBinaryClientSecureChannel& secure_channel_;
  scada::NodeId authentication_token_;
  std::uint32_t next_request_handle_ = 1;
  std::unordered_map<std::uint32_t, BufferedResponse> buffered_responses_;
};

}  // namespace opcua
