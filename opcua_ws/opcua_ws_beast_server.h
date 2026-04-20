#pragma once

#include "opcua_ws/opcua_ws_beast_transport.h"
#include "opcua_ws/opcua_ws_server.h"

#include <boost/asio/ip/tcp.hpp>

#include <string>
#include <vector>

namespace opcua_ws {

struct OpcUaWsBeastServerContext {
  transport::executor executor;
  boost::asio::ip::tcp::endpoint endpoint;
  OpcUaWsRuntime& runtime;
  size_t max_message_size = 4 * 1024 * 1024;
  std::string path = "/ua";
  std::string subprotocol = "opcua+uajson";
  std::vector<std::string> allowed_origins;
  bool enable_permessage_deflate = true;
  std::string server_header = "scada-opcua-ws";
};

class OpcUaWsBeastServer : private OpcUaWsBeastServerContext {
 public:
  explicit OpcUaWsBeastServer(OpcUaWsBeastServerContext&& context);

  [[nodiscard]] transport::awaitable<transport::error_code> open();
  [[nodiscard]] transport::awaitable<transport::error_code> close();
  [[nodiscard]] boost::asio::ip::tcp::endpoint local_endpoint() const;

 private:
  [[nodiscard]] transport::awaitable<void> AcceptLoop();
  [[nodiscard]] transport::awaitable<void> HandleSocket(
      boost::asio::ip::tcp::socket socket);

  boost::asio::ip::tcp::acceptor acceptor_;
  OpcUaWsServer server_;
  bool opened_ = false;
};

}  // namespace opcua_ws
