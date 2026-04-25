#pragma once

#include "base/awaitable.h"
#include "opcua/client_connection.h"
#include "opcua/message.h"
#include "scada/status.h"
#include "scada/status_or.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace opcua {

// Request-response correlation layer shared by OPC UA client transports.
class ClientChannel {
 public:
  struct Context {
    ClientConnection& connection;
    // Authentication token from a successful CreateSession; empty node id
    // before session activation.
    scada::NodeId authentication_token;
  };

  explicit ClientChannel(Context context);

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
  [[nodiscard]] Awaitable<scada::StatusOr<ResponseBody>> Call(
      std::uint32_t request_handle,
      RequestBody request);

  // Lower-level split send/receive API for callers that keep multiple
  // requests outstanding (Publish). `Receive` buffers unrelated responses
  // for later matching by request_id.
  [[nodiscard]] Awaitable<scada::StatusOr<std::uint32_t>> Send(
      std::uint32_t request_handle,
      RequestBody request);
  [[nodiscard]] Awaitable<scada::StatusOr<ResponseBody>> Receive(
      std::uint32_t request_id,
      std::uint32_t request_handle);

 private:
  struct BufferedResponse {
    std::uint32_t request_handle = 0;
    ResponseBody body;
  };

  ClientConnection& connection_;
  scada::NodeId authentication_token_;
  std::uint32_t next_request_handle_ = 1;
  std::unordered_map<std::uint32_t, BufferedResponse> buffered_responses_;
};

}  // namespace opcua
