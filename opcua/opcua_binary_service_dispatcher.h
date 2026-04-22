#pragma once

#include "base/awaitable.h"
#include "opcua/opcua_binary_runtime.h"
#include "opcua/opcua_binary_session_service.h"
#include "opcua_ws/opcua_ws_session_manager.h"

namespace opcua {

class OpcUaBinaryServiceDispatcher {
 public:
  struct Context {
    OpcUaBinaryRuntime& runtime;
    opcua_ws::OpcUaWsSessionManager& session_manager;
    OpcUaBinaryConnectionState& connection;
  };

  explicit OpcUaBinaryServiceDispatcher(Context context);

  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandlePayload(
      std::vector<char> payload);

 private:
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandleReadPayload(
      std::vector<char> payload);

  OpcUaBinaryRuntime& runtime_;
  opcua_ws::OpcUaWsSessionManager& session_manager_;
  OpcUaBinaryConnectionState& connection_;
  OpcUaBinarySessionService session_service_;
};

}  // namespace opcua
