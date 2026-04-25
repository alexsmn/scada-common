#pragma once

#include "base/awaitable.h"
#include "opcua/binary/protocol.h"
#include "scada/status.h"
#include "scada/status_or.h"

#include <transport/any_transport.h>
#include <transport/write_queue.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace opcua {

struct OpcUaBinaryClientTransportContext {
  transport::any_transport transport;
  std::string endpoint_url;
  OpcUaBinaryTransportLimits limits;
  std::size_t read_buffer_size = 64 * 1024;
  std::size_t max_frame_size = 16 * 1024 * 1024;
};

// Client-side analogue of OpcUaBinaryTcpConnection. Owns a transport that the
// caller has already constructed (typically through transport::TransportFactory
// from an "opc.tcp://" URL), drives the OPC UA Part 6 HEL/ACK negotiation,
// and then exposes a raw frame read/write API for the secure channel layer to
// sit on top of. No SecureChannel logic lives here.
class OpcUaBinaryClientTransport {
 public:
  explicit OpcUaBinaryClientTransport(
      OpcUaBinaryClientTransportContext&& context);

  [[nodiscard]] Awaitable<scada::Status> Connect();

  [[nodiscard]] Awaitable<scada::StatusOr<std::vector<char>>> ReadFrame();
  [[nodiscard]] Awaitable<scada::Status> WriteFrame(
      const std::vector<char>& frame);

  [[nodiscard]] Awaitable<void> Close();

  [[nodiscard]] const OpcUaBinaryAcknowledgeMessage& acknowledge() const {
    return acknowledge_;
  }

  [[nodiscard]] bool is_open() const { return open_; }

 private:
  transport::any_transport transport_;
  const std::string endpoint_url_;
  const OpcUaBinaryTransportLimits limits_;
  const std::size_t read_buffer_size_;
  const std::size_t max_frame_size_;
  transport::WriteQueue write_queue_;

  bool open_ = false;
  OpcUaBinaryAcknowledgeMessage acknowledge_{};
  std::vector<char> pending_bytes_;
};

}  // namespace opcua
