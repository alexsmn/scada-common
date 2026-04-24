#include "opcua/binary/client/opcua_binary_client_connection.h"

#include "opcua/binary/opcua_binary_service_codec.h"

#include <utility>

namespace opcua {

OpcUaBinaryClientConnection::OpcUaBinaryClientConnection(Context context)
    : transport_{context.transport},
      secure_channel_{context.secure_channel} {}

Awaitable<scada::Status> OpcUaBinaryClientConnection::Open() {
  auto connect_status = co_await transport_.Connect();
  if (connect_status.bad()) {
    co_return connect_status;
  }
  co_return co_await secure_channel_.Open();
}

Awaitable<scada::Status> OpcUaBinaryClientConnection::Close() {
  (void)(co_await secure_channel_.Close());
  co_await transport_.Close();
  co_return scada::Status{scada::StatusCode::Good};
}

std::uint32_t OpcUaBinaryClientConnection::NextRequestId() {
  return secure_channel_.NextRequestId();
}

Awaitable<scada::Status> OpcUaBinaryClientConnection::SendRequest(
    std::uint32_t request_id,
    const OpcUaRequestMessage& message,
    const scada::NodeId& authentication_token) {
  const OpcUaBinaryServiceRequestHeader header{
      .authentication_token = authentication_token,
      .request_handle = message.request_handle,
  };
  const auto body = EncodeOpcUaBinaryServiceRequest(header, message.body);
  if (!body.has_value()) {
    co_return scada::Status{scada::StatusCode::Bad};
  }
  co_return co_await secure_channel_.SendServiceRequest(request_id, *body);
}

Awaitable<scada::StatusOr<OpcUaClientResponseFrame>>
OpcUaBinaryClientConnection::ReadResponse() {
  auto response_frame = co_await secure_channel_.ReadServiceResponse();
  if (!response_frame.ok()) {
    co_return scada::StatusOr<OpcUaClientResponseFrame>{
        response_frame.status()};
  }

  auto decoded = DecodeOpcUaBinaryServiceResponse(response_frame->body);
  if (!decoded.has_value()) {
    co_return scada::StatusOr<OpcUaClientResponseFrame>{
        scada::Status{scada::StatusCode::Bad}};
  }

  co_return scada::StatusOr<OpcUaClientResponseFrame>{
      OpcUaClientResponseFrame{
          .request_id = response_frame->request_id,
          .message = OpcUaResponseMessage{
              .request_handle = decoded->request_handle,
              .body = std::move(decoded->body),
          },
      }};
}

}  // namespace opcua
