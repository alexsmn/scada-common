#pragma once

#include "base/awaitable.h"
#include "opcua/binary/opcua_binary_protocol.h"
#include "opcua/binary/opcua_binary_secure_channel.h"

#include <transport/any_transport.h>
#include <transport/write_queue.h>

#include <functional>
#include <optional>
#include <vector>

namespace opcua {

using OpcUaBinarySecureFrameHandler =
    std::function<Awaitable<std::optional<std::vector<char>>>(std::vector<char>)>;

struct OpcUaBinaryTcpConnectionContext {
  transport::any_transport transport;
  OpcUaBinaryTransportLimits limits;
  std::size_t read_buffer_size = 64 * 1024;
  std::size_t max_frame_size = 16 * 1024 * 1024;
  OpcUaBinarySecureFrameHandler on_secure_frame =
      [](std::vector<char>) -> Awaitable<std::optional<std::vector<char>>> {
    co_return std::nullopt;
  };
};

class OpcUaBinaryTcpConnection : private OpcUaBinaryTcpConnectionContext {
 public:
  explicit OpcUaBinaryTcpConnection(OpcUaBinaryTcpConnectionContext&& context);

  [[nodiscard]] Awaitable<void> Run();

 private:
  [[nodiscard]] Awaitable<bool> ProcessBufferedFrames(
      transport::WriteQueue& write_queue,
      std::vector<char>& pending_bytes);
  [[nodiscard]] Awaitable<bool> ProcessFrame(transport::WriteQueue& write_queue,
                                            const std::vector<char>& frame);
  [[nodiscard]] Awaitable<bool> WriteErrorAndClose(
      transport::WriteQueue& write_queue,
      scada::Status error,
      std::string reason);

  bool hello_received_ = false;
  OpcUaBinarySecureChannel secure_channel_;
};

}  // namespace opcua
