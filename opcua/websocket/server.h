#pragma once

#include "opcua/server_runtime.h"

#include <transport/any_transport.h>
#include <transport/error.h>

namespace opcua::ws {

struct ServerContext {
  transport::any_transport acceptor;
  ServerRuntime& runtime;
  size_t max_message_size = 4 * 1024 * 1024;
};

class Server : private ServerContext {
 public:
  explicit Server(ServerContext&& context);

  [[nodiscard]] Awaitable<transport::error_code> Open();
  [[nodiscard]] Awaitable<transport::error_code> Close();
  [[nodiscard]] Awaitable<void> ServeConnection(transport::any_transport transport);

 private:
  [[nodiscard]] Awaitable<void> AcceptLoop();
  [[nodiscard]] Awaitable<void> RunConnection(transport::any_transport transport);

  bool opened_ = false;
};

}  // namespace opcua::ws
