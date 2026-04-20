#pragma once

#include "transport/transport.h"

#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/core/tcp_stream.hpp>

namespace opcua_ws {

class OpcUaWsBeastTransport final : public transport::Transport {
 public:
  using WebSocketStream =
      boost::beast::websocket::stream<boost::beast::tcp_stream>;

  explicit OpcUaWsBeastTransport(WebSocketStream websocket);

  [[nodiscard]] transport::awaitable<transport::error_code> open() override;
  [[nodiscard]] transport::awaitable<transport::error_code> close() override;
  [[nodiscard]] transport::awaitable<transport::expected<transport::any_transport>>
  accept() override;
  [[nodiscard]] transport::awaitable<transport::expected<size_t>> read(
      std::span<char> data) override;
  [[nodiscard]] transport::awaitable<transport::expected<size_t>> write(
      std::span<const char> data) override;

  [[nodiscard]] std::string name() const override { return "OpcUaWsBeast"; }
  [[nodiscard]] bool message_oriented() const override { return true; }
  [[nodiscard]] bool connected() const override { return opened_; }
  [[nodiscard]] bool active() const override { return false; }
  [[nodiscard]] transport::executor get_executor() override;

 private:
  WebSocketStream websocket_;
  bool opened_ = false;
};

}  // namespace opcua_ws
