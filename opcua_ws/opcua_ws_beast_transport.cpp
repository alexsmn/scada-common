#include "opcua_ws/opcua_ws_beast_transport.h"

#include "transport/any_transport.h"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <cstring>

namespace opcua_ws {

OpcUaWsBeastTransport::OpcUaWsBeastTransport(WebSocketStream websocket)
    : websocket_{std::move(websocket)} {}

transport::awaitable<transport::error_code> OpcUaWsBeastTransport::open() {
  opened_ = true;
  co_return transport::OK;
}

transport::awaitable<transport::error_code> OpcUaWsBeastTransport::close() {
  if (!opened_)
    co_return transport::OK;

  auto [ec] = co_await websocket_.async_close(
      boost::beast::websocket::close_code::normal,
      boost::asio::as_tuple(boost::asio::use_awaitable));
  opened_ = false;
  if (ec == boost::beast::websocket::error::closed)
    co_return transport::OK;
  co_return ec;
}

transport::awaitable<transport::expected<transport::any_transport>>
OpcUaWsBeastTransport::accept() {
  co_return transport::ERR_ACCESS_DENIED;
}

transport::awaitable<transport::expected<size_t>> OpcUaWsBeastTransport::read(
    std::span<char> data) {
  boost::beast::flat_buffer buffer;
  auto [ec, _] = co_await websocket_.async_read(
      buffer, boost::asio::as_tuple(boost::asio::use_awaitable));
  if (ec == boost::beast::websocket::error::closed) {
    opened_ = false;
    co_return size_t{0};
  }
  if (ec)
    co_return ec;

  const auto size = buffer.size();
  if (size > data.size())
    co_return transport::ERR_INVALID_ARGUMENT;

  auto buffer_data = buffer.data();
  std::memcpy(data.data(), buffer_data.data(), size);
  co_return size;
}

transport::awaitable<transport::expected<size_t>> OpcUaWsBeastTransport::write(
    std::span<const char> data) {
  websocket_.text(true);
  auto [ec, written] = co_await websocket_.async_write(
      boost::asio::buffer(data.data(), data.size()),
      boost::asio::as_tuple(boost::asio::use_awaitable));
  if (ec)
    co_return ec;
  co_return written;
}

transport::executor OpcUaWsBeastTransport::get_executor() {
  return websocket_.get_executor();
}

}  // namespace opcua_ws
