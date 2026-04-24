#include "opcua/binary/client/opcua_binary_client_channel.h"

#include <utility>

namespace opcua {

OpcUaBinaryClientChannel::OpcUaBinaryClientChannel(Context context)
    : transport_{context.transport},
      secure_channel_{context.secure_channel},
      authentication_token_{std::move(context.authentication_token)} {}

std::uint32_t OpcUaBinaryClientChannel::NextRequestHandle() {
  return next_request_handle_++;
}

void OpcUaBinaryClientChannel::set_authentication_token(scada::NodeId token) {
  authentication_token_ = std::move(token);
}

Awaitable<scada::StatusOr<OpcUaResponseBody>>
OpcUaBinaryClientChannel::Call(std::uint32_t request_handle,
                               OpcUaRequestBody request) {
  auto request_id = co_await Send(request_handle, std::move(request));
  if (!request_id.ok()) {
    co_return scada::StatusOr<OpcUaResponseBody>{request_id.status()};
  }
  co_return co_await Receive(*request_id, request_handle);
}

Awaitable<scada::StatusOr<std::uint32_t>> OpcUaBinaryClientChannel::Send(
    std::uint32_t request_handle,
    OpcUaRequestBody request) {
  const OpcUaBinaryServiceRequestHeader header{
      .authentication_token = authentication_token_,
      .request_handle = request_handle,
  };
  const auto body =
      EncodeOpcUaBinaryServiceRequest(header, request);
  if (!body.has_value()) {
    co_return scada::StatusOr<std::uint32_t>{
        scada::Status{scada::StatusCode::Bad}};
  }

  const std::uint32_t request_id = secure_channel_.NextRequestId();
  const auto send_status =
      co_await secure_channel_.SendServiceRequest(request_id, *body);
  if (send_status.bad()) {
    co_return scada::StatusOr<std::uint32_t>{send_status};
  }
  co_return scada::StatusOr<std::uint32_t>{request_id};
}

Awaitable<scada::StatusOr<OpcUaResponseBody>> OpcUaBinaryClientChannel::Receive(
    std::uint32_t request_id,
    std::uint32_t request_handle) {
  if (auto it = buffered_responses_.find(request_id);
      it != buffered_responses_.end()) {
    if (it->second.request_handle != request_handle) {
      co_return scada::StatusOr<OpcUaResponseBody>{
          scada::Status{scada::StatusCode::Bad}};
    }
    auto body = std::move(it->second.body);
    buffered_responses_.erase(it);
    co_return scada::StatusOr<OpcUaResponseBody>{std::move(body)};
  }

  for (;;) {
    auto response_frame = co_await secure_channel_.ReadServiceResponse();
    if (!response_frame.ok()) {
      co_return scada::StatusOr<OpcUaResponseBody>{response_frame.status()};
    }

    auto decoded = DecodeOpcUaBinaryServiceResponse(response_frame->body);
    if (!decoded.has_value()) {
      co_return scada::StatusOr<OpcUaResponseBody>{
          scada::Status{scada::StatusCode::Bad}};
    }

    if (response_frame->request_id == request_id) {
      if (decoded->request_handle != request_handle) {
        co_return scada::StatusOr<OpcUaResponseBody>{
            scada::Status{scada::StatusCode::Bad}};
      }
      co_return scada::StatusOr<OpcUaResponseBody>{
          std::move(decoded->body)};
    }

    buffered_responses_.emplace(
        response_frame->request_id,
        BufferedResponse{.request_handle = decoded->request_handle,
                         .body = std::move(decoded->body)});
  }
}

}  // namespace opcua
