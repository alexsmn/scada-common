#pragma once

#include "base/awaitable.h"
#include "opcua/message.h"
#include "scada/status.h"
#include "scada/status_or.h"

#include <cstdint>

namespace opcua {

struct OpcUaClientResponseFrame {
  std::uint32_t request_id = 0;
  OpcUaResponseMessage message;
};

// Transport/protocol adapter used by the reusable OPC UA client layer.
// Binary TCP and future WebSocket clients provide concrete implementations
// that translate typed request/response messages to their wire format.
class OpcUaClientConnection {
 public:
  OpcUaClientConnection() = default;
  OpcUaClientConnection(const OpcUaClientConnection&) = delete;
  OpcUaClientConnection& operator=(const OpcUaClientConnection&) = delete;
  virtual ~OpcUaClientConnection() = default;

  [[nodiscard]] virtual Awaitable<scada::Status> Open() = 0;
  [[nodiscard]] virtual Awaitable<scada::Status> Close() = 0;

  [[nodiscard]] virtual std::uint32_t NextRequestId() = 0;
  [[nodiscard]] virtual Awaitable<scada::Status> SendRequest(
      std::uint32_t request_id,
      const OpcUaRequestMessage& message,
      const scada::NodeId& authentication_token) = 0;
  [[nodiscard]] virtual Awaitable<scada::StatusOr<OpcUaClientResponseFrame>>
  ReadResponse() = 0;
};

}  // namespace opcua
