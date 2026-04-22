#pragma once

#include "base/awaitable.h"
#include "opcua/opcua_binary_runtime.h"

#include <transport/any_transport.h>
#include <transport/error.h>

namespace opcua {

struct OpcUaBinaryServerContext {
  transport::any_transport acceptor;
  OpcUaBinaryRuntime& runtime;
  OpcUaSessionManager& session_manager;
  std::size_t read_buffer_size = 64 * 1024;
  std::size_t max_frame_size = 16 * 1024 * 1024;
};

class OpcUaBinaryServer : private OpcUaBinaryServerContext {
 public:
  explicit OpcUaBinaryServer(OpcUaBinaryServerContext&& context);

  [[nodiscard]] Awaitable<transport::error_code> Open();
  [[nodiscard]] Awaitable<transport::error_code> Close();
  [[nodiscard]] Awaitable<void> ServeConnection(transport::any_transport transport);

 private:
  [[nodiscard]] Awaitable<void> AcceptLoop();
  [[nodiscard]] Awaitable<void> RunConnection(transport::any_transport transport);

  bool opened_ = false;
};

}  // namespace opcua
