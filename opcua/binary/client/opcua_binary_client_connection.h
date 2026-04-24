#pragma once

#include "opcua/binary/client/opcua_binary_client_secure_channel.h"
#include "opcua/binary/client/opcua_binary_client_transport.h"
#include "opcua/client/opcua_client_connection.h"

namespace opcua {

class OpcUaBinaryClientConnection final : public OpcUaClientConnection {
 public:
  struct Context {
    OpcUaBinaryClientTransport& transport;
    OpcUaBinaryClientSecureChannel& secure_channel;
  };

  explicit OpcUaBinaryClientConnection(Context context);

  [[nodiscard]] Awaitable<scada::Status> Open() override;
  [[nodiscard]] Awaitable<scada::Status> Close() override;

  [[nodiscard]] std::uint32_t NextRequestId() override;
  [[nodiscard]] Awaitable<scada::Status> SendRequest(
      std::uint32_t request_id,
      const OpcUaRequestMessage& message,
      const scada::NodeId& authentication_token) override;
  [[nodiscard]] Awaitable<scada::StatusOr<OpcUaClientResponseFrame>>
  ReadResponse() override;

 private:
  OpcUaBinaryClientTransport& transport_;
  OpcUaBinaryClientSecureChannel& secure_channel_;
};

}  // namespace opcua
