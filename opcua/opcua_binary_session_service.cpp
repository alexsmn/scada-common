#include "opcua/opcua_binary_session_service.h"
#include "opcua/opcua_binary_codec_utils.h"

namespace opcua {
namespace {

constexpr std::uint32_t kCreateSessionResponseBinaryEncodingId = 464;
constexpr std::uint32_t kActivateSessionResponseBinaryEncodingId = 470;
constexpr std::uint32_t kCloseSessionResponseBinaryEncodingId = 476;

using binary::AppendByteString;
using binary::AppendDouble;
using binary::AppendInt32;
using binary::AppendInt64;
using binary::AppendMessage;
using binary::AppendNumericNodeId;
using binary::AppendUInt8;
using binary::AppendUInt32;

void AppendResponseHeader(std::vector<char>& bytes,
                          std::uint32_t request_handle,
                          scada::Status status) {
  AppendInt64(bytes, 0);
  AppendUInt32(bytes, request_handle);
  AppendUInt32(bytes, status.full_code());
  AppendUInt8(bytes, 0);
  AppendInt32(bytes, 0);
  AppendNumericNodeId(bytes, scada::NodeId{});
  AppendUInt8(bytes, 0x00);
}

std::vector<char> EncodeCreateSessionResponseBody(
    std::uint32_t request_handle,
    const OpcUaBinaryCreateSessionResponse& response) {
  std::vector<char> payload;
  AppendResponseHeader(payload, request_handle, response.status);
  AppendNumericNodeId(payload, response.session_id);
  AppendNumericNodeId(payload, response.authentication_token);
  AppendDouble(payload, response.revised_timeout.InMillisecondsF());
  AppendByteString(payload, response.server_nonce);
  AppendByteString(payload, {});
  AppendInt32(payload, -1);
  AppendInt32(payload, -1);
  binary::AppendUaString(payload, "");
  AppendByteString(payload, {});
  AppendUInt32(payload, 0);

  std::vector<char> body;
  AppendMessage(body, kCreateSessionResponseBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeActivateSessionResponseBody(
    std::uint32_t request_handle,
    const OpcUaBinaryActivateSessionResponse& response) {
  std::vector<char> payload;
  AppendResponseHeader(payload, request_handle, response.status);
  AppendByteString(payload, {});
  AppendInt32(payload, -1);
  AppendInt32(payload, -1);

  std::vector<char> body;
  AppendMessage(body, kActivateSessionResponseBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeCloseSessionResponseBody(
    std::uint32_t request_handle,
    const OpcUaBinaryCloseSessionResponse& response) {
  std::vector<char> payload;
  AppendResponseHeader(payload, request_handle, response.status);

  std::vector<char> body;
  AppendMessage(body, kCloseSessionResponseBinaryEncodingId, payload);
  return body;
}

}  // namespace

OpcUaBinarySessionService::OpcUaBinarySessionService(Context context)
    : runtime_{context.runtime},
      session_manager_{context.session_manager},
      connection_{context.connection} {}

Awaitable<std::optional<std::vector<char>>> OpcUaBinarySessionService::HandleRequest(
    OpcUaBinaryDecodedRequest request) {
  if (const auto* create =
          std::get_if<OpcUaBinaryCreateSessionRequest>(&request.body)) {
    auto response = co_await runtime_.CreateSession(*create);
    co_return EncodeCreateSessionResponseBody(request.header.request_handle,
                                              response);
  }

  if (const auto* activate =
          std::get_if<OpcUaBinaryActivateSessionRequest>(&request.body)) {
    auto runtime_request = *activate;
    const auto session =
        session_manager_.FindSession(request.header.authentication_token);
    if (!session.has_value()) {
      co_return std::nullopt;
    }
    runtime_request.session_id = session->session_id;
    runtime_request.authentication_token = request.header.authentication_token;
    auto response =
        co_await runtime_.ActivateSession(connection_, std::move(runtime_request));
    co_return EncodeActivateSessionResponseBody(request.header.request_handle,
                                                response);
  }

  if (std::holds_alternative<OpcUaBinaryCloseSessionRequest>(request.body)) {
    const auto session =
        session_manager_.FindSession(request.header.authentication_token);
    if (!session.has_value()) {
      co_return EncodeCloseSessionResponseBody(
          request.header.request_handle,
          {.status = scada::StatusCode::Bad_SessionIsLoggedOff});
    }
    auto response = co_await runtime_.CloseSession(
        connection_,
        {.session_id = session->session_id,
         .authentication_token = request.header.authentication_token});
    co_return EncodeCloseSessionResponseBody(request.header.request_handle,
                                             response);
  }

  co_return std::nullopt;
}

}  // namespace opcua
