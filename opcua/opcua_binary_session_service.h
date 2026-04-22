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
      OpcUaBinaryDecodedRequest request);

 private:
  OpcUaBinaryRuntime& runtime_;
  OpcUaSessionManager& session_manager_;
  OpcUaBinaryConnectionState& connection_;
};

}  // namespace opcua
