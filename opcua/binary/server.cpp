#include "opcua/binary/server.h"

#include "opcua/binary/service_dispatcher.h"
#include "opcua/binary/tcp_connection.h"

#include <memory>

namespace opcua::binary {
namespace {

struct ConnectionTaskState {
  explicit ConnectionTaskState(transport::any_transport transport)
      : transport{std::move(transport)} {}

  transport::any_transport transport;
  ConnectionState connection;
};

}  // namespace

Server::Server(ServerContext&& context)
    : ServerContext{std::move(context)} {}

Awaitable<transport::error_code> Server::Open() {
  if (opened_) {
    co_return transport::OK;
  }

  auto error = co_await acceptor.open();
  if (error) {
    co_return error;
  }

  opened_ = true;
  CoSpawn(acceptor.get_executor(),
          [this]() -> Awaitable<void> { co_await AcceptLoop(); });
  co_return transport::OK;
}

Awaitable<transport::error_code> Server::Close() {
  if (!opened_) {
    co_return transport::OK;
  }

  opened_ = false;
  co_return co_await acceptor.close();
}

Awaitable<void> Server::ServeConnection(
    transport::any_transport transport) {
  co_await RunConnection(std::move(transport));
}

Awaitable<void> Server::AcceptLoop() {
  while (opened_) {
    auto accepted = co_await acceptor.accept();
    if (!accepted.ok()) {
      co_return;
    }

    auto transport = std::move(accepted.value());
    auto executor = transport.get_executor();
    CoSpawn(executor,
            [this, transport = std::move(transport)]() mutable -> Awaitable<void> {
              co_await ServeConnection(std::move(transport));
            });
  }
}

Awaitable<void> Server::RunConnection(transport::any_transport transport) {
  auto state = std::make_shared<ConnectionTaskState>(std::move(transport));
  ServiceDispatcher dispatcher{
      {.runtime = runtime,
       .connection = state->connection}};
  co_await TcpConnection{
      {.transport = std::move(state->transport),
       .read_buffer_size = this->read_buffer_size,
       .max_frame_size = max_frame_size,
       .on_secure_frame =
           [&dispatcher](std::vector<char> payload)
               -> Awaitable<std::optional<std::vector<char>>> {
         co_return co_await dispatcher.HandlePayload(std::move(payload));
       }}}
      .Run();
  state->connection.closed = true;
  runtime.Detach(state->connection);
}

}  // namespace opcua::binary
