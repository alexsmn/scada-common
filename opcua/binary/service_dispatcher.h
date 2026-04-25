#pragma once

#include "base/awaitable.h"
#include "opcua/binary/service_codec.h"
#include "opcua/binary/runtime.h"
#include "opcua/server_session_manager.h"

namespace opcua {

class OpcUaBinaryServiceDispatcher {
 public:
  struct Context {
    OpcUaBinaryRuntime& runtime;
    OpcUaBinaryConnectionState& connection;
  };

  explicit OpcUaBinaryServiceDispatcher(Context context);

  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandlePayload(
      std::vector<char> payload);

 private:
  OpcUaBinaryRuntime& runtime_;
  OpcUaBinaryConnectionState& connection_;
};

}  // namespace opcua
