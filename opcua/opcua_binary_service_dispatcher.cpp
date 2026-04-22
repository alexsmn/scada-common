#include "opcua/opcua_binary_service_dispatcher.h"
#include "opcua/opcua_binary_codec_utils.h"

#include "scada/localized_text.h"

#include <cstring>
#include <tuple>

namespace opcua {
namespace {

constexpr std::uint32_t kCreateSessionRequestBinaryEncodingId = 461;
constexpr std::uint32_t kActivateSessionRequestBinaryEncodingId = 467;
constexpr std::uint32_t kCloseSessionRequestBinaryEncodingId = 473;
constexpr std::uint32_t kReadRequestBinaryEncodingId = 629;
constexpr std::uint32_t kReadResponseBinaryEncodingId = 632;
constexpr std::uint32_t kWriteRequestBinaryEncodingId = 671;
constexpr std::uint32_t kWriteResponseBinaryEncodingId = 674;

enum class OpcUaBinaryTimestampsToReturn : std::uint32_t {
  Source = 0,
  Server = 1,
  Both = 2,
  Neither = 3,
};

struct WireRequestHeader {
  scada::NodeId authentication_token;
  std::uint32_t request_handle = 0;
};

using binary::AppendDouble;
using binary::AppendInt32;
using binary::AppendInt64;
using binary::AppendMessage;
using binary::AppendNumericNodeId;
using binary::AppendUaString;
using binary::AppendUInt8;
using binary::AppendUInt16;
using binary::AppendUInt32;
using binary::ReadDouble;
using binary::ReadInt32;
using binary::ReadInt64;
using binary::ReadMessage;
using binary::ReadNumericNodeId;
using binary::ReadUaString;
using binary::ReadUInt8;
using binary::ReadUInt16;
using binary::ReadUInt32;

bool SkipQualifiedName(const std::vector<char>& bytes, std::size_t& offset) {
  std::uint16_t namespace_index = 0;
  std::string name;
  return ReadUInt16(bytes, offset, namespace_index) &&
         ReadUaString(bytes, offset, name);
}

bool ReadRequestHeader(const std::vector<char>& bytes,
                       std::size_t& offset,
                       WireRequestHeader& header) {
  std::int64_t ignored_timestamp = 0;
  if (!ReadNumericNodeId(bytes, offset, header.authentication_token) ||
      !ReadInt64(bytes, offset, ignored_timestamp) ||
      !ReadUInt32(bytes, offset, header.request_handle)) {
    return false;
  }

  std::uint32_t ignored_u32 = 0;
  std::string ignored_string;
  if (!ReadUInt32(bytes, offset, ignored_u32) ||
      !ReadUaString(bytes, offset, ignored_string) ||
      !ReadUInt32(bytes, offset, ignored_u32)) {
    return false;
  }

  scada::NodeId ignored_node_id;
  std::uint8_t ignored_byte = 0;
  return ReadNumericNodeId(bytes, offset, ignored_node_id) &&
         ReadUInt8(bytes, offset, ignored_byte);
}

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

void AppendLocalizedText(std::vector<char>& bytes, const scada::LocalizedText& text) {
  const auto utf8 = ToString(text);
  const std::uint8_t mask = utf8.empty() ? 0 : 0x02;
  AppendUInt8(bytes, mask);
  if ((mask & 0x02) != 0) {
    AppendUaString(bytes, utf8);
  }
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

std::optional<scada::Variant> DecodeVariant(const std::vector<char>& bytes,
                                            std::size_t& offset) {
  std::uint8_t encoding_mask = 0;
  if (!ReadUInt8(bytes, offset, encoding_mask)) {
    return std::nullopt;
  }

  if ((encoding_mask & 0x80) != 0) {
    return std::nullopt;
  }

  switch (encoding_mask) {
    case 0:
      return scada::Variant{};

    case 6: {
      std::int32_t value = 0;
      if (!ReadInt32(bytes, offset, value)) {
        return std::nullopt;
      }
      return scada::Variant{value};
    }

    case 7: {
      std::uint32_t value = 0;
      if (!ReadUInt32(bytes, offset, value)) {
        return std::nullopt;
      }
      return scada::Variant{value};
    }

    case 11: {
      double value = 0;
      if (!ReadDouble(bytes, offset, value)) {
        return std::nullopt;
      }
      return scada::Variant{value};
    }

    case 12: {
      std::string value;
      if (!ReadUaString(bytes, offset, value)) {
        return std::nullopt;
      }
      return scada::Variant{std::move(value)};
    }

    default:
      return std::nullopt;
  }
}

std::optional<scada::Variant> DecodeDataValue(const std::vector<char>& bytes,
                                              std::size_t& offset) {
  std::uint8_t mask = 0;
  if (!ReadUInt8(bytes, offset, mask)) {
    return std::nullopt;
  }

  if ((mask & 0xf0) != 0) {
    return std::nullopt;
  }

  scada::Variant value;
  if ((mask & 0x01) != 0) {
    auto decoded = DecodeVariant(bytes, offset);
    if (!decoded.has_value()) {
      return std::nullopt;
    }
    value = std::move(*decoded);
  }

  if ((mask & 0x02) != 0) {
    std::uint32_t ignored_status = 0;
    if (!ReadUInt32(bytes, offset, ignored_status)) {
      return std::nullopt;
    }
  }
  if ((mask & 0x04) != 0) {
    std::int64_t ignored_source_timestamp = 0;
    if (!ReadInt64(bytes, offset, ignored_source_timestamp)) {
      return std::nullopt;
    }
  }
  if ((mask & 0x08) != 0) {
    std::int64_t ignored_server_timestamp = 0;
    if (!ReadInt64(bytes, offset, ignored_server_timestamp)) {
      return std::nullopt;
    }
  }

  return value;
}

bool DecodeWriteValue(const std::vector<char>& bytes,
                      std::size_t& offset,
                      scada::WriteValue& value) {
  std::uint32_t attribute_id = 0;
  std::string index_range;
  if (!ReadNumericNodeId(bytes, offset, value.node_id) ||
      !ReadUInt32(bytes, offset, attribute_id) ||
      !ReadUaString(bytes, offset, index_range)) {
    return false;
  }

  auto decoded = DecodeDataValue(bytes, offset);
  if (!decoded.has_value()) {
    return false;
  }

  value.attribute_id = static_cast<scada::AttributeId>(attribute_id);
  value.value = std::move(*decoded);
  return index_range.empty();
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

bool DecodeReadValueId(const std::vector<char>& bytes,
                       std::size_t& offset,
                       scada::ReadValueId& value_id) {
  std::uint32_t attribute_id = 0;
  std::string index_range;
  if (!ReadNumericNodeId(bytes, offset, value_id.node_id) ||
      !ReadUInt32(bytes, offset, attribute_id) ||
      !ReadUaString(bytes, offset, index_range) ||
      !SkipQualifiedName(bytes, offset)) {
    return false;
  }
  value_id.attribute_id = static_cast<scada::AttributeId>(attribute_id);
  return index_range.empty();
}

std::optional<std::tuple<std::uint32_t, WireRequestHeader, OpcUaBinaryReadRequest>>
DecodeReadRequest(const std::vector<char>& payload) {
  std::size_t offset = 0;
  scada::NodeId type_node_id;
  if (!ReadNumericNodeId(payload, offset, type_node_id) ||
      !type_node_id.is_numeric()) {
    return std::nullopt;
  }

  const auto type_id = type_node_id.numeric_id();
  if (type_id != kReadRequestBinaryEncodingId) {
    return std::nullopt;
  }

  WireRequestHeader header;
  double max_age = 0;
  std::uint32_t timestamps_to_return = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(payload, offset, header) ||
      !ReadDouble(payload, offset, max_age) ||
      !ReadUInt32(payload, offset, timestamps_to_return) ||
      !ReadInt32(payload, offset, count) || count < 0) {
    return std::nullopt;
  }

  if (max_age < 0) {
    return std::nullopt;
  }

  const auto timestamps =
      static_cast<OpcUaBinaryTimestampsToReturn>(timestamps_to_return);
  if (timestamps != OpcUaBinaryTimestampsToReturn::Source &&
      timestamps != OpcUaBinaryTimestampsToReturn::Server &&
      timestamps != OpcUaBinaryTimestampsToReturn::Both &&
      timestamps != OpcUaBinaryTimestampsToReturn::Neither) {
    return std::nullopt;
  }

  OpcUaBinaryReadRequest request;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeReadValueId(payload, offset, input)) {
      return std::nullopt;
    }
  }
  if (offset != payload.size()) {
    return std::nullopt;
  }

  return std::tuple{type_id, header, std::move(request)};
}

std::optional<std::tuple<std::uint32_t, WireRequestHeader, OpcUaBinaryWriteRequest>>
DecodeWriteRequest(const std::vector<char>& payload) {
  std::size_t offset = 0;
  scada::NodeId type_node_id;
  if (!ReadNumericNodeId(payload, offset, type_node_id) ||
      !type_node_id.is_numeric() ||
      type_node_id.numeric_id() != kWriteRequestBinaryEncodingId) {
    return std::nullopt;
  }

  WireRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(payload, offset, header) ||
      !ReadInt32(payload, offset, count) || count < 0) {
    return std::nullopt;
  }

  OpcUaBinaryWriteRequest request;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeWriteValue(payload, offset, input)) {
      return std::nullopt;
    }
  }

  if (offset != payload.size()) {
    return std::nullopt;
  }

  return std::tuple{kWriteRequestBinaryEncodingId, header, std::move(request)};
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
  std::size_t offset = 0;
  scada::NodeId type_node_id;
  if (!ReadNumericNodeId(payload, offset, type_node_id) ||
      !type_node_id.is_numeric()) {
    co_return std::nullopt;
  }

  switch (type_node_id.numeric_id()) {
    case kCreateSessionRequestBinaryEncodingId:
    case kActivateSessionRequestBinaryEncodingId:
    case kCloseSessionRequestBinaryEncodingId:
      co_return co_await session_service_.HandlePayload(std::move(payload));
    case kReadRequestBinaryEncodingId:
      co_return co_await HandleReadPayload(std::move(payload));
    case kWriteRequestBinaryEncodingId:
      co_return co_await HandleWritePayload(std::move(payload));
    default:
      co_return std::nullopt;
  }
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleReadPayload(std::vector<char> payload) {
  const auto decoded = DecodeReadRequest(payload);
  if (!decoded.has_value()) {
    co_return std::nullopt;
  }

  const auto& [type_id, header, request] = *decoded;
  (void)type_id;

  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != header.authentication_token) {
    co_return EncodeReadResponseBody(
        header.request_handle,
        {.status = scada::StatusCode::Bad_SessionIsLoggedOff});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryReadResponse>(connection_, request);
  co_return EncodeReadResponseBody(header.request_handle, response);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleWritePayload(std::vector<char> payload) {
  const auto decoded = DecodeWriteRequest(payload);
  if (!decoded.has_value()) {
    co_return std::nullopt;
  }

  const auto& [type_id, header, request] = *decoded;
  (void)type_id;

  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != header.authentication_token) {
    co_return EncodeWriteResponseBody(
        header.request_handle,
        {.status = scada::StatusCode::Bad_SessionIsLoggedOff});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryWriteResponse>(connection_, request);
  co_return EncodeWriteResponseBody(header.request_handle, response);
}

}  // namespace opcua
