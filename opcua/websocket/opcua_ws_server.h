#pragma once

#include "opcua/websocket/opcua_ws_runtime.h"

#include <transport/any_transport.h>
#include <transport/error.h>

namespace opcua_ws {

struct OpcUaWsServerContext {
  transport::any_transport acceptor;
  OpcUaWsRuntime& runtime;
  size_t max_message_size = 4 * 1024 * 1024;
};

class OpcUaWsServer : private OpcUaWsServerContext {
 public:
  explicit OpcUaWsServer(OpcUaWsServerContext&& context);

  [[nodiscard]] Awaitable<transport::error_code> Open();
  [[nodiscard]] Awaitable<transport::error_code> Close();
  [[nodiscard]] Awaitable<void> ServeConnection(transport::any_transport transport);

 private:
  [[nodiscard]] Awaitable<void> AcceptLoop();
  [[nodiscard]] Awaitable<void> RunConnection(transport::any_transport transport);

  bool opened_ = false;
};

}  // namespace opcua_ws
