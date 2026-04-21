#include "opcua_ws/opcua_ws_server.h"

#include "base/awaitable.h"
#include "opcua_ws/opcua_json_codec.h"

#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>

#include <string>
#include <vector>

namespace opcua_ws {

namespace {

template <typename T>
std::span<const char> AsCharSpan(const T& container) {
  return {reinterpret_cast<const char*>(container.data()), container.size()};
}

}  // namespace

OpcUaWsServer::OpcUaWsServer(OpcUaWsServerContext&& context)
    : OpcUaWsServerContext{std::move(context)} {}

Awaitable<transport::error_code> OpcUaWsServer::Open() {
  if (opened_)
    co_return transport::OK;

  auto error = co_await acceptor.open();
  if (error)
    co_return error;

  opened_ = true;
  CoSpawn(acceptor.get_executor(),
          [this]() -> Awaitable<void> { co_await AcceptLoop(); });
  co_return transport::OK;
}

Awaitable<transport::error_code> OpcUaWsServer::Close() {
  if (!opened_)
    co_return transport::OK;

  opened_ = false;
  co_return co_await acceptor.close();
}

Awaitable<void> OpcUaWsServer::ServeConnection(transport::any_transport transport) {
  co_await RunConnection(std::move(transport));
}

Awaitable<void> OpcUaWsServer::AcceptLoop() {
  while (opened_) {
    auto accepted = co_await acceptor.accept();
    if (!accepted.ok())
      co_return;

    auto transport = std::move(accepted.value());
    auto executor = transport.get_executor();
    CoSpawn(executor,
            [this, transport = std::move(transport)]() mutable -> Awaitable<void> {
              co_await ServeConnection(std::move(transport));
            });
  }
}

Awaitable<void> OpcUaWsServer::RunConnection(transport::any_transport transport) {
  [[maybe_unused]] auto open_result = co_await transport.open();

  OpcUaWsConnectionState connection;
  std::vector<char> buffer(max_message_size);

  for (;;) {
    auto read_result = co_await transport.read(buffer);
    if (!read_result.ok() || *read_result == 0)
      break;

    OpcUaWsResponseMessage response;
    try {
      const std::string_view payload{buffer.data(), *read_result};
      response = co_await runtime.Handle(
          connection, DecodeRequestMessage(boost::json::parse(payload)));
    } catch (...) {
      response = {.request_handle = 0,
                  .body = OpcUaWsServiceFault{
                  .status = scada::StatusCode::Bad_CantParseString}};
    }

    auto encoded = boost::json::serialize(EncodeJson(response));
    if (encoded.size() > max_message_size)
      break;

    auto write_result =
        co_await transport.write(AsCharSpan(encoded));
    if (!write_result.ok())
      break;
  }

  runtime.Detach(connection);
  [[maybe_unused]] auto close_result = co_await transport.close();
}

}  // namespace opcua_ws
