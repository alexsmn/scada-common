#include "opcua_ws/opcua_ws_beast_server.h"

#include "opcua_ws/opcua_ws_tls_context.h"
#include "transport/websocket_transport.h"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/http.hpp>

#include <algorithm>
#include <cctype>

namespace opcua_ws {
namespace {

namespace http = boost::beast::http;
namespace websocket = boost::beast::websocket;

std::string Trim(std::string_view value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
    value.remove_prefix(1);
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
    value.remove_suffix(1);
  return std::string{value};
}

bool HeaderContainsToken(std::string_view header, std::string_view token) {
  while (!header.empty()) {
    const auto comma = header.find(',');
    const auto part = comma == std::string_view::npos ? header
                                                      : header.substr(0, comma);
    if (Trim(part) == token)
      return true;
    if (comma == std::string_view::npos)
      break;
    header.remove_prefix(comma + 1);
  }
  return false;
}

transport::awaitable<void> RejectRequest(boost::asio::ip::tcp::socket& socket,
                                         http::status status,
                                         std::string body) {
  http::response<http::string_body> response{status, 11};
  response.set(http::field::content_type, "text/plain");
  response.body() = std::move(body);
  response.prepare_payload();
  auto [ec, _] = co_await http::async_write(
      socket, response, boost::asio::as_tuple(boost::asio::use_awaitable));
  if (!ec) {
    boost::system::error_code ignored;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
  }
  co_return;
}

template <typename Stream>
transport::awaitable<void> RejectRequest(Stream& stream,
                                         http::status status,
                                         std::string body) {
  http::response<http::string_body> response{status, 11};
  response.set(http::field::content_type, "text/plain");
  response.body() = std::move(body);
  response.prepare_payload();
  auto [ec, _] = co_await http::async_write(
      stream, response, boost::asio::as_tuple(boost::asio::use_awaitable));
  if (!ec) {
    boost::system::error_code ignored;
    if constexpr (std::is_same_v<Stream, boost::asio::ip::tcp::socket>) {
      stream.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
    } else {
      stream.next_layer().shutdown(
          boost::asio::ip::tcp::socket::shutdown_both, ignored);
    }
  }
  co_return;
}

}  // namespace

OpcUaWsBeastServer::OpcUaWsBeastServer(OpcUaWsBeastServerContext&& context)
    : OpcUaWsBeastServerContext{std::move(context)},
      acceptor_{this->executor},
      server_{{.acceptor = transport::any_transport{},
               .runtime = this->runtime,
               .max_message_size = this->max_message_size}} {}

transport::awaitable<transport::error_code> OpcUaWsBeastServer::open() {
  if (opened_)
    co_return transport::OK;

  if (this->tls.has_value()) {
    ssl_context_.emplace(boost::asio::ssl::context::tls_server);
    const auto tls_error =
        ConfigureServerTlsContext(*ssl_context_, *this->tls);
    if (tls_error != transport::OK)
      co_return tls_error;
  }

  boost::system::error_code ec;
  acceptor_.open(this->endpoint.protocol(), ec);
  if (ec)
    co_return ec;
  acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec)
    co_return ec;
  acceptor_.bind(this->endpoint, ec);
  if (ec)
    co_return ec;
  acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec)
    co_return ec;

  opened_ = true;
  boost::asio::co_spawn(
      this->executor, [this]() { return AcceptLoop(); }, boost::asio::detached);
  co_return transport::OK;
}

transport::awaitable<transport::error_code> OpcUaWsBeastServer::close() {
  opened_ = false;
  boost::system::error_code ec;
  acceptor_.cancel(ec);
  acceptor_.close(ec);
  co_return ec;
}

boost::asio::ip::tcp::endpoint OpcUaWsBeastServer::local_endpoint() const {
  boost::system::error_code ec;
  return acceptor_.local_endpoint(ec);
}

transport::awaitable<void> OpcUaWsBeastServer::AcceptLoop() {
  while (opened_) {
    auto [ec, socket] = co_await acceptor_.async_accept(
        boost::asio::as_tuple(boost::asio::use_awaitable));
    if (ec)
      co_return;

    boost::asio::co_spawn(
        socket.get_executor(),
        [this, socket = std::move(socket)]() mutable {
          return HandleSocket(std::move(socket));
        },
        boost::asio::detached);
  }
}

transport::awaitable<void> OpcUaWsBeastServer::HandleSocket(
    boost::asio::ip::tcp::socket socket) {
  if (ssl_context_.has_value()) {
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> tls_stream{
        std::move(socket), *ssl_context_};
    auto [tls_ec] = co_await tls_stream.async_handshake(
        boost::asio::ssl::stream_base::server,
        boost::asio::as_tuple(boost::asio::use_awaitable));
    if (tls_ec)
      co_return;

    co_await HandleUpgradedStream(
        boost::beast::websocket::stream<
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>{
            std::move(tls_stream)});
    co_return;
  }

  co_await HandleUpgradedStream(
      boost::beast::websocket::stream<boost::asio::ip::tcp::socket>{
          std::move(socket)});
}

template <typename NextLayer>
transport::awaitable<void> OpcUaWsBeastServer::HandleUpgradedStream(
    boost::beast::websocket::stream<NextLayer> websocket) {
  auto& next_layer = websocket.next_layer();

  boost::beast::flat_buffer buffer;
  http::request<http::string_body> request;
  auto [read_ec, _] = co_await http::async_read(next_layer, buffer, request,
                                                boost::asio::as_tuple(
                                                    boost::asio::use_awaitable));
  if (read_ec)
    co_return;

  if (request.target() != this->path) {
    co_await RejectRequest(next_layer, http::status::not_found, "Invalid target");
    co_return;
  }

  if (!websocket::is_upgrade(request)) {
    co_await RejectRequest(next_layer, http::status::bad_request,
                           "WebSocket upgrade required");
    co_return;
  }

  const auto subprotocol_it = request.find(http::field::sec_websocket_protocol);
  if (subprotocol_it == request.end() ||
      !HeaderContainsToken(subprotocol_it->value(), this->subprotocol)) {
    co_await RejectRequest(next_layer, http::status::bad_request,
                           "Missing required subprotocol");
    co_return;
  }

  if (!this->allowed_origins.empty()) {
    const auto origin_it = request.find(http::field::origin);
    if (origin_it == request.end()) {
      co_await RejectRequest(next_layer, http::status::forbidden,
                             "Origin required");
      co_return;
    }

    const auto origin = std::string{origin_it->value()};
    const auto allowed = std::find(this->allowed_origins.begin(),
                                   this->allowed_origins.end(),
                                   origin) != this->allowed_origins.end();
    if (!allowed) {
      co_await RejectRequest(next_layer, http::status::forbidden, "Origin denied");
      co_return;
    }
  }

  websocket.set_option(
      websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
  if (this->enable_permessage_deflate) {
    websocket::permessage_deflate options;
    options.server_enable = true;
    options.client_enable = true;
    websocket.set_option(options);
  }
  websocket.set_option(websocket::stream_base::decorator(
      [this](websocket::response_type& response) {
        response.set(http::field::server, this->server_header);
        response.set(http::field::sec_websocket_protocol, this->subprotocol);
      }));

  auto [accept_ec] = co_await websocket.async_accept(
      request, boost::asio::as_tuple(boost::asio::use_awaitable));
  if (accept_ec)
    co_return;

  co_await server_.ServeConnection(
      transport::any_transport{std::make_unique<transport::WebSocketTransport>(
          std::move(websocket))});
}

}  // namespace opcua_ws
