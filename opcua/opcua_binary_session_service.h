#pragma once

#include "base/awaitable.h"
#include "opcua/opcua_binary_service_codec.h"
#include "opcua/opcua_binary_runtime.h"
#include "opcua/opcua_server_session_manager.h"

namespace opcua {

class OpcUaBinarySessionService {
 public:
  struct Context {
    OpcUaBinaryRuntime& runtime;
    OpcUaSessionManager& session_manager;
    OpcUaBinaryConnectionState& connection;
  };

  explicit OpcUaBinarySessionService(Context context);

  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandleRequest(
      scada::UInt32 request_handle,
      OpcUaBinaryCreateSessionRequest request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandleRequest(
      const OpcUaBinaryServiceRequestHeader& header,
      OpcUaBinaryActivateSessionRequest request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandleCloseRequest(
      scada::UInt32 request_handle,
      const scada::NodeId& authentication_token);

 private:
  OpcUaBinaryRuntime& runtime_;
  OpcUaSessionManager& session_manager_;
  OpcUaBinaryConnectionState& connection_;
};

}  // namespace opcua
