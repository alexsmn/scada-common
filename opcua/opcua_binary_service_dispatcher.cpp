#include "opcua/opcua_binary_service_dispatcher.h"
#include "opcua/opcua_binary_codec_utils.h"

#include "scada/localized_text.h"

namespace opcua {
namespace {

constexpr std::uint32_t kBrowseRequestBinaryEncodingId = 525;
constexpr std::uint32_t kBrowseResponseBinaryEncodingId = 528;
constexpr std::uint32_t kReadRequestBinaryEncodingId = 629;
constexpr std::uint32_t kReadResponseBinaryEncodingId = 632;
constexpr std::uint32_t kWriteRequestBinaryEncodingId = 671;
constexpr std::uint32_t kWriteResponseBinaryEncodingId = 674;

using binary::AppendDouble;
using binary::AppendInt32;
using binary::AppendInt64;
using binary::AppendBoolean;
using binary::AppendByteString;
using binary::AppendExpandedNodeId;
using binary::AppendLocalizedText;
using binary::AppendMessage;
using binary::AppendNumericNodeId;
using binary::AppendQualifiedName;
using binary::AppendUaString;
using binary::AppendUInt8;
using binary::AppendUInt16;
using binary::AppendUInt32;
using binary::ReadByteString;
using binary::ReadExpandedNodeId;
using binary::ReadInt64;
using binary::ReadLocalizedText;
using binary::ReadMessage;
using binary::ReadNumericNodeId;
using binary::ReadQualifiedName;
using binary::ReadUInt8;
using binary::ReadUInt32;

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

std::uint32_t EncodeStatusCode(scada::StatusCode status_code) {
  return static_cast<std::uint32_t>(status_code) << 16;
}

void AppendVariant(std::vector<char>& bytes, const scada::Variant& value) {
  if (value.is_null()) {
    AppendUInt8(bytes, 0);
    return;
  }
  if (const auto* double_value = value.get_if<scada::Double>()) {
    AppendUInt8(bytes, 11);
    AppendDouble(bytes, *double_value);
    return;
  }
  if (const auto* int32_value = value.get_if<scada::Int32>()) {
    AppendUInt8(bytes, 6);
    AppendInt32(bytes, *int32_value);
    return;
  }
  if (const auto* uint32_value = value.get_if<scada::UInt32>()) {
    AppendUInt8(bytes, 7);
    AppendUInt32(bytes, *uint32_value);
    return;
  }
  if (const auto* string_value = value.get_if<scada::String>()) {
    AppendUInt8(bytes, 12);
    AppendUaString(bytes, *string_value);
    return;
  }
  if (const auto* localized_text = value.get_if<scada::LocalizedText>()) {
    AppendUInt8(bytes, 21);
    AppendLocalizedText(bytes, *localized_text);
    return;
  }

  // Keep the initial binary adapter explicit about the supported scalar set.
  AppendUInt8(bytes, 0);
}

void AppendDateTime(std::vector<char>& bytes, scada::DateTime value) {
  if (value.is_null()) {
    AppendInt64(bytes, 0);
    return;
  }
  AppendInt64(bytes, value.ToDeltaSinceWindowsEpoch().InMicroseconds() * 10);
}

void AppendDataValue(std::vector<char>& bytes, const scada::DataValue& value) {
  std::uint8_t mask = 0;
  if (!value.value.is_null()) {
    mask |= 0x01;
  }
  if (!scada::IsGood(value.status_code)) {
    mask |= 0x02;
  }
  if (!value.source_timestamp.is_null()) {
    mask |= 0x04;
  }
  if (!value.server_timestamp.is_null()) {
    mask |= 0x08;
  }

  AppendUInt8(bytes, mask);
  if ((mask & 0x01) != 0) {
    AppendVariant(bytes, value.value);
  }
  if ((mask & 0x02) != 0) {
    AppendUInt32(bytes, EncodeStatusCode(value.status_code));
  }
  if ((mask & 0x04) != 0) {
    AppendDateTime(bytes, value.source_timestamp);
  }
  if ((mask & 0x08) != 0) {
    AppendDateTime(bytes, value.server_timestamp);
  }
}

std::vector<char> EncodeReadResponseBody(std::uint32_t request_handle,
                                         const OpcUaBinaryReadResponse& response) {
  std::vector<char> payload;
  AppendResponseHeader(payload, request_handle, response.status);
  AppendInt32(payload, static_cast<std::int32_t>(response.results.size()));
  for (const auto& result : response.results) {
    AppendDataValue(payload, result);
  }
  AppendInt32(payload, -1);

  std::vector<char> body;
  AppendMessage(body, kReadResponseBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeWriteResponseBody(
    std::uint32_t request_handle,
    const OpcUaBinaryWriteResponse& response) {
  std::vector<char> payload;
  AppendResponseHeader(payload, request_handle, response.status);
  AppendInt32(payload, static_cast<std::int32_t>(response.results.size()));
  for (const auto& result : response.results) {
    AppendUInt32(payload, EncodeStatusCode(result));
  }
  AppendInt32(payload, -1);

  std::vector<char> body;
  AppendMessage(body, kWriteResponseBinaryEncodingId, payload);
  return body;
}

void AppendReferenceDescription(std::vector<char>& bytes,
                                const scada::ReferenceDescription& reference) {
  AppendNumericNodeId(bytes, reference.reference_type_id);
  AppendBoolean(bytes, reference.forward);
  AppendExpandedNodeId(bytes, scada::ExpandedNodeId{reference.node_id});
  AppendQualifiedName(bytes, scada::QualifiedName{});
  AppendLocalizedText(bytes, scada::LocalizedText{});
  AppendUInt32(bytes, 0);
  AppendExpandedNodeId(bytes, scada::ExpandedNodeId{});
}

void AppendBrowseResult(std::vector<char>& bytes,
                        const scada::BrowseResult& result) {
  AppendUInt32(bytes, EncodeStatusCode(result.status_code));
  AppendByteString(bytes, result.continuation_point);
  AppendInt32(bytes, static_cast<std::int32_t>(result.references.size()));
  for (const auto& reference : result.references) {
    AppendReferenceDescription(bytes, reference);
  }
}

std::vector<char> EncodeBrowseResponseBody(
    std::uint32_t request_handle,
    const OpcUaBinaryBrowseResponse& response) {
  std::vector<char> payload;
  AppendResponseHeader(payload, request_handle, response.status);
  AppendInt32(payload, static_cast<std::int32_t>(response.results.size()));
  for (const auto& result : response.results) {
    AppendBrowseResult(payload, result);
  }
  AppendInt32(payload, -1);

  std::vector<char> body;
  AppendMessage(body, kBrowseResponseBinaryEncodingId, payload);
  return body;
}

}  // namespace

OpcUaBinaryServiceDispatcher::OpcUaBinaryServiceDispatcher(Context context)
    : runtime_{context.runtime},
      session_manager_{context.session_manager},
      connection_{context.connection},
      session_service_{{.runtime = context.runtime,
                        .session_manager = context.session_manager,
                        .connection = context.connection}} {}

Awaitable<std::optional<std::vector<char>>> OpcUaBinaryServiceDispatcher::HandlePayload(
    std::vector<char> payload) {
  const auto request = DecodeOpcUaBinaryServiceRequest(payload);
  if (!request.has_value()) {
    co_return std::nullopt;
  }

  if (std::holds_alternative<OpcUaBinaryCreateSessionRequest>(request->body) ||
      std::holds_alternative<OpcUaBinaryActivateSessionRequest>(request->body) ||
      std::holds_alternative<OpcUaBinaryCloseSessionRequest>(request->body)) {
    co_return co_await session_service_.HandleRequest(*request);
  }
  if (std::holds_alternative<OpcUaBinaryBrowseRequest>(request->body)) {
    co_return co_await HandleBrowseRequest(*request);
  }
  if (std::holds_alternative<OpcUaBinaryReadRequest>(request->body)) {
    co_return co_await HandleReadRequest(*request);
  }
  if (std::holds_alternative<OpcUaBinaryWriteRequest>(request->body)) {
    co_return co_await HandleWriteRequest(*request);
  }
  co_return std::nullopt;
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleBrowseRequest(
    const OpcUaBinaryDecodedRequest& request) {
  const auto* browse = std::get_if<OpcUaBinaryBrowseRequest>(&request.body);
  if (browse == nullptr) {
    co_return std::nullopt;
  }
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeBrowseResponseBody(
        request.header.request_handle,
        {.status = scada::StatusCode::Bad_SessionIsLoggedOff});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryBrowseResponse>(connection_, *browse);
  co_return EncodeBrowseResponseBody(request.header.request_handle, response);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleReadRequest(
    const OpcUaBinaryDecodedRequest& request) {
  const auto* read = std::get_if<OpcUaBinaryReadRequest>(&request.body);
  if (read == nullptr) {
    co_return std::nullopt;
  }
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeReadResponseBody(
        request.header.request_handle,
        {.status = scada::StatusCode::Bad_SessionIsLoggedOff});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryReadResponse>(connection_, *read);
  co_return EncodeReadResponseBody(request.header.request_handle, response);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleWriteRequest(
    const OpcUaBinaryDecodedRequest& request) {
  const auto* write = std::get_if<OpcUaBinaryWriteRequest>(&request.body);
  if (write == nullptr) {
    co_return std::nullopt;
  }
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeWriteResponseBody(
        request.header.request_handle,
        {.status = scada::StatusCode::Bad_SessionIsLoggedOff});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryWriteResponse>(connection_, *write);
  co_return EncodeWriteResponseBody(request.header.request_handle, response);
}

}  // namespace opcua
