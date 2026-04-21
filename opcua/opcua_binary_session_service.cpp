#include "opcua/opcua_binary_session_service.h"

#include "scada/localized_text.h"

#include <cstring>

namespace opcua {
namespace {

constexpr std::uint32_t kCreateSessionRequestBinaryEncodingId = 461;
constexpr std::uint32_t kCreateSessionResponseBinaryEncodingId = 464;
constexpr std::uint32_t kActivateSessionRequestBinaryEncodingId = 467;
constexpr std::uint32_t kActivateSessionResponseBinaryEncodingId = 470;
constexpr std::uint32_t kCloseSessionRequestBinaryEncodingId = 473;
constexpr std::uint32_t kCloseSessionResponseBinaryEncodingId = 476;
constexpr std::uint32_t kAnonymousIdentityTokenBinaryEncodingId = 321;
constexpr std::uint32_t kUserNameIdentityTokenBinaryEncodingId = 324;

struct WireRequestHeader {
  scada::NodeId authentication_token;
  std::uint32_t request_handle = 0;
};

void AppendUInt8(std::vector<char>& bytes, std::uint8_t value) {
  bytes.push_back(static_cast<char>(value));
}

void AppendUInt16(std::vector<char>& bytes, std::uint16_t value) {
  bytes.push_back(static_cast<char>(value & 0xff));
  bytes.push_back(static_cast<char>((value >> 8) & 0xff));
}

void AppendUInt32(std::vector<char>& bytes, std::uint32_t value) {
  bytes.push_back(static_cast<char>(value & 0xff));
  bytes.push_back(static_cast<char>((value >> 8) & 0xff));
  bytes.push_back(static_cast<char>((value >> 16) & 0xff));
  bytes.push_back(static_cast<char>((value >> 24) & 0xff));
}

void AppendInt32(std::vector<char>& bytes, std::int32_t value) {
  AppendUInt32(bytes, static_cast<std::uint32_t>(value));
}

void AppendInt64(std::vector<char>& bytes, std::int64_t value) {
  const auto raw = static_cast<std::uint64_t>(value);
  for (int i = 0; i < 8; ++i) {
    bytes.push_back(static_cast<char>((raw >> (8 * i)) & 0xff));
  }
}

void AppendDouble(std::vector<char>& bytes, double value) {
  const auto* raw = reinterpret_cast<const char*>(&value);
  bytes.insert(bytes.end(), raw, raw + sizeof(value));
}

bool ReadUInt8(const std::vector<char>& bytes,
               std::size_t& offset,
               std::uint8_t& value) {
  if (offset + 1 > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint8_t>(bytes[offset]);
  ++offset;
  return true;
}

bool ReadUInt16(const std::vector<char>& bytes,
                std::size_t& offset,
                std::uint16_t& value) {
  if (offset + 2 > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint16_t>(
      static_cast<unsigned char>(bytes[offset]) |
      (static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[offset + 1]))
       << 8));
  offset += 2;
  return true;
}

bool ReadUInt32(const std::vector<char>& bytes,
                std::size_t& offset,
                std::uint32_t& value) {
  if (offset + 4 > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint32_t>(
              static_cast<unsigned char>(bytes[offset])) |
          (static_cast<std::uint32_t>(
               static_cast<unsigned char>(bytes[offset + 1]))
           << 8) |
          (static_cast<std::uint32_t>(
               static_cast<unsigned char>(bytes[offset + 2]))
           << 16) |
          (static_cast<std::uint32_t>(
               static_cast<unsigned char>(bytes[offset + 3]))
           << 24);
  offset += 4;
  return true;
}

bool ReadInt32(const std::vector<char>& bytes,
               std::size_t& offset,
               std::int32_t& value) {
  std::uint32_t raw = 0;
  if (!ReadUInt32(bytes, offset, raw)) {
    return false;
  }
  value = static_cast<std::int32_t>(raw);
  return true;
}

bool ReadInt64(const std::vector<char>& bytes,
               std::size_t& offset,
               std::int64_t& value) {
  if (offset + 8 > bytes.size()) {
    return false;
  }
  std::uint64_t raw = 0;
  for (int i = 0; i < 8; ++i) {
    raw |= static_cast<std::uint64_t>(
               static_cast<unsigned char>(bytes[offset + i]))
           << (8 * i);
  }
  value = static_cast<std::int64_t>(raw);
  offset += 8;
  return true;
}

bool ReadDouble(const std::vector<char>& bytes, std::size_t& offset, double& value) {
  if (offset + sizeof(double) > bytes.size()) {
    return false;
  }
  std::memcpy(&value, bytes.data() + offset, sizeof(value));
  offset += sizeof(value);
  return true;
}

void AppendUaString(std::vector<char>& bytes, std::string_view value) {
  AppendInt32(bytes, static_cast<std::int32_t>(value.size()));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

bool ReadUaString(const std::vector<char>& bytes,
                  std::size_t& offset,
                  std::string& value) {
  std::int32_t length = 0;
  if (!ReadInt32(bytes, offset, length)) {
    return false;
  }
  if (length < 0) {
    value.clear();
    return true;
  }
  if (offset + static_cast<std::size_t>(length) > bytes.size()) {
    return false;
  }
  value.assign(bytes.data() + offset, static_cast<std::size_t>(length));
  offset += static_cast<std::size_t>(length);
  return true;
}

void AppendByteString(std::vector<char>& bytes, const scada::ByteString& value) {
  AppendInt32(bytes, static_cast<std::int32_t>(value.size()));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

bool ReadByteString(const std::vector<char>& bytes,
                    std::size_t& offset,
                    scada::ByteString& value) {
  std::int32_t length = 0;
  if (!ReadInt32(bytes, offset, length)) {
    return false;
  }
  if (length < 0) {
    value.clear();
    return true;
  }
  if (offset + static_cast<std::size_t>(length) > bytes.size()) {
    return false;
  }
  value.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
               bytes.begin() + static_cast<std::ptrdiff_t>(offset + length));
  offset += static_cast<std::size_t>(length);
  return true;
}

void AppendNumericNodeId(std::vector<char>& bytes, const scada::NodeId& node_id) {
  if (node_id.is_null()) {
    AppendUInt8(bytes, 0x00);
    return;
  }
  if (!node_id.is_numeric()) {
    AppendUInt8(bytes, 0x00);
    return;
  }
  if (node_id.namespace_index() <= 0xff && node_id.numeric_id() <= 0xffff) {
    AppendUInt8(bytes, 0x01);
    AppendUInt8(bytes, static_cast<std::uint8_t>(node_id.namespace_index()));
    AppendUInt16(bytes, static_cast<std::uint16_t>(node_id.numeric_id()));
    return;
  }
  AppendUInt8(bytes, 0x02);
  AppendUInt16(bytes, node_id.namespace_index());
  AppendUInt32(bytes, node_id.numeric_id());
}

bool ReadNumericNodeId(const std::vector<char>& bytes,
                       std::size_t& offset,
                       scada::NodeId& id) {
  std::uint8_t encoding = 0;
  if (!ReadUInt8(bytes, offset, encoding)) {
    return false;
  }
  if (encoding == 0x00) {
    id = {};
    return true;
  }
  if (encoding == 0x01) {
    std::uint8_t ns = 0;
    std::uint16_t short_id = 0;
    if (!ReadUInt8(bytes, offset, ns) || !ReadUInt16(bytes, offset, short_id)) {
      return false;
    }
    id = scada::NodeId{short_id, ns};
    return true;
  }
  if (encoding == 0x02) {
    std::uint16_t ns = 0;
    std::uint32_t numeric_id = 0;
    if (!ReadUInt16(bytes, offset, ns) || !ReadUInt32(bytes, offset, numeric_id)) {
      return false;
    }
    id = scada::NodeId{numeric_id, ns};
    return true;
  }
  return false;
}

bool SkipStringArray(const std::vector<char>& bytes, std::size_t& offset) {
  std::int32_t count = 0;
  if (!ReadInt32(bytes, offset, count)) {
    return false;
  }
  if (count < 0) {
    return true;
  }
  for (std::int32_t i = 0; i < count; ++i) {
    std::string ignored;
    if (!ReadUaString(bytes, offset, ignored)) {
      return false;
    }
  }
  return true;
}

void AppendExtensionObject(std::vector<char>& bytes,
                           std::uint32_t type_id,
                           const std::vector<char>& body) {
  AppendNumericNodeId(bytes, scada::NodeId{type_id});
  AppendUInt8(bytes, 0x01);
  AppendInt32(bytes, static_cast<std::int32_t>(body.size()));
  bytes.insert(bytes.end(), body.begin(), body.end());
}

bool ReadExtensionObject(const std::vector<char>& bytes,
                         std::size_t& offset,
                         std::uint32_t& type_id,
                         std::uint8_t& encoding,
                         std::vector<char>& body) {
  scada::NodeId type_node_id;
  if (!ReadNumericNodeId(bytes, offset, type_node_id) ||
      !type_node_id.is_numeric() ||
      !ReadUInt8(bytes, offset, encoding)) {
    return false;
  }
  type_id = type_node_id.numeric_id();
  if (encoding == 0x00) {
    body.clear();
    return true;
  }
  std::int32_t length = 0;
  if (!ReadInt32(bytes, offset, length) || length < 0 ||
      offset + static_cast<std::size_t>(length) > bytes.size()) {
    return false;
  }
  body.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
              bytes.begin() + static_cast<std::ptrdiff_t>(offset + length));
  offset += static_cast<std::size_t>(length);
  return true;
}

bool SkipLocalizedText(const std::vector<char>& bytes, std::size_t& offset) {
  std::uint8_t mask = 0;
  if (!ReadUInt8(bytes, offset, mask)) {
    return false;
  }
  if ((mask & 0x01) != 0) {
    std::string locale;
    if (!ReadUaString(bytes, offset, locale)) {
      return false;
    }
  }
  if ((mask & 0x02) != 0) {
    std::string text;
    if (!ReadUaString(bytes, offset, text)) {
      return false;
    }
  }
  return true;
}

bool SkipApplicationDescriptionFields(const std::vector<char>& bytes,
                                      std::size_t& offset) {
  std::string ignored;
  std::int32_t application_type = 0;
  return ReadUaString(bytes, offset, ignored) &&
         ReadUaString(bytes, offset, ignored) &&
         SkipLocalizedText(bytes, offset) &&
         ReadInt32(bytes, offset, application_type) &&
         ReadUaString(bytes, offset, ignored) &&
         ReadUaString(bytes, offset, ignored) &&
         SkipStringArray(bytes, offset);
}

bool SkipSignatureData(const std::vector<char>& bytes, std::size_t& offset) {
  std::string algorithm;
  scada::ByteString signature;
  return ReadUaString(bytes, offset, algorithm) &&
         ReadByteString(bytes, offset, signature);
}

bool SkipSignedSoftwareCertificates(const std::vector<char>& bytes,
                                    std::size_t& offset) {
  std::int32_t count = 0;
  if (!ReadInt32(bytes, offset, count)) {
    return false;
  }
  if (count < 0) {
    return true;
  }
  for (std::int32_t i = 0; i < count; ++i) {
    scada::ByteString certificate_data;
    scada::ByteString signature;
    if (!ReadByteString(bytes, offset, certificate_data) ||
        !ReadByteString(bytes, offset, signature)) {
      return false;
    }
  }
  return true;
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

  std::uint32_t additional_type_id = 0;
  std::uint8_t additional_encoding = 0;
  std::vector<char> additional_body;
  return ReadExtensionObject(bytes, offset, additional_type_id,
                             additional_encoding, additional_body);
}

void AppendMessage(std::vector<char>& bytes,
                   std::uint32_t type_id,
                   const std::vector<char>& body) {
  AppendNumericNodeId(bytes, scada::NodeId{type_id});
  bytes.insert(bytes.end(), body.begin(), body.end());
}

void AppendResponseHeader(std::vector<char>& bytes,
                          std::uint32_t request_handle,
                          scada::Status status) {
  AppendInt64(bytes, 0);
  AppendUInt32(bytes, request_handle);
  AppendUInt32(bytes, status.good() ? 0u : 0x80000000u);
  AppendUInt8(bytes, 0);
  AppendInt32(bytes, 0);
  AppendNumericNodeId(bytes, scada::NodeId{});
  AppendUInt8(bytes, 0x00);
}

std::optional<opcua_ws::OpcUaWsActivateSessionRequest> DecodeActivateRequest(
    const std::vector<char>& payload,
    opcua_ws::OpcUaWsSessionManager& session_manager,
    std::uint32_t& request_handle) {
  std::size_t offset = 0;
  WireRequestHeader header;
  if (!ReadRequestHeader(payload, offset, header) ||
      !header.authentication_token.is_numeric()) {
    return std::nullopt;
  }
  request_handle = header.request_handle;

  if (!SkipSignatureData(payload, offset) ||
      !SkipSignedSoftwareCertificates(payload, offset) ||
      !SkipStringArray(payload, offset)) {
    return std::nullopt;
  }

  std::uint32_t user_identity_type_id = 0;
  std::uint8_t user_identity_encoding = 0;
  std::vector<char> user_identity_body;
  if (!ReadExtensionObject(payload, offset, user_identity_type_id,
                           user_identity_encoding, user_identity_body) ||
      user_identity_encoding != 0x01 ||
      !SkipSignatureData(payload, offset) || offset != payload.size()) {
    return std::nullopt;
  }

  const auto session = session_manager.FindSession(header.authentication_token);
  if (!session.has_value()) {
    return std::nullopt;
  }

  opcua_ws::OpcUaWsActivateSessionRequest request{
      .session_id = session->session_id,
      .authentication_token = header.authentication_token,
  };

  std::size_t body_offset = 0;
  std::string ignored_policy_id;
  if (!ReadUaString(user_identity_body, body_offset, ignored_policy_id)) {
    return std::nullopt;
  }
  if (user_identity_type_id == kAnonymousIdentityTokenBinaryEncodingId) {
    request.allow_anonymous = true;
  } else if (user_identity_type_id == kUserNameIdentityTokenBinaryEncodingId) {
    std::string user_name;
    scada::ByteString password;
    std::string encryption_algorithm;
    if (!ReadUaString(user_identity_body, body_offset, user_name) ||
        !ReadByteString(user_identity_body, body_offset, password) ||
        !ReadUaString(user_identity_body, body_offset, encryption_algorithm) ||
        body_offset != user_identity_body.size() ||
        !encryption_algorithm.empty()) {
      return std::nullopt;
    }
    request.user_name = scada::ToLocalizedText(user_name);
    request.password = scada::ToLocalizedText(
        std::string{password.begin(), password.end()});
  } else {
    return std::nullopt;
  }

  return request;
}

std::optional<WireRequestHeader> DecodeCloseRequestHeader(
    const std::vector<char>& payload) {
  std::size_t offset = 0;
  WireRequestHeader header;
  std::uint8_t delete_subscriptions = 0;
  if (!ReadRequestHeader(payload, offset, header) ||
      !ReadUInt8(payload, offset, delete_subscriptions) ||
      offset != payload.size()) {
    return std::nullopt;
  }
  return header;
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
  AppendUaString(payload, "");
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

Awaitable<std::optional<std::vector<char>>> OpcUaBinarySessionService::HandlePayload(
    std::vector<char> payload) {
  std::size_t offset = 0;
  scada::NodeId type_node_id;
  if (!ReadNumericNodeId(payload, offset, type_node_id) ||
      !type_node_id.is_numeric()) {
    co_return std::nullopt;
  }

  const auto type_id = type_node_id.numeric_id();
  std::vector<char> body{payload.begin() + static_cast<std::ptrdiff_t>(offset),
                         payload.end()};

  if (type_id == kCreateSessionRequestBinaryEncodingId) {
    WireRequestHeader header;
    std::size_t body_offset = 0;
    if (!ReadRequestHeader(body, body_offset, header) ||
        !SkipApplicationDescriptionFields(body, body_offset)) {
      co_return std::nullopt;
    }
    std::string ignored;
    scada::ByteString ignored_bytes;
    double requested_timeout_ms = 0;
    std::uint32_t ignored_max_response_size = 0;
    if (!ReadUaString(body, body_offset, ignored) ||
        !ReadUaString(body, body_offset, ignored) ||
        !ReadUaString(body, body_offset, ignored) ||
        !ReadByteString(body, body_offset, ignored_bytes) ||
        !ReadByteString(body, body_offset, ignored_bytes) ||
        !ReadDouble(body, body_offset, requested_timeout_ms) ||
        !ReadUInt32(body, body_offset, ignored_max_response_size) ||
        body_offset != body.size()) {
      co_return std::nullopt;
    }

    auto response = co_await runtime_.CreateSession(
        {.requested_timeout =
             base::TimeDelta::FromMillisecondsD(requested_timeout_ms)});
    co_return EncodeCreateSessionResponseBody(header.request_handle, response);
  }

  if (type_id == kActivateSessionRequestBinaryEncodingId) {
    std::uint32_t request_handle = 0;
    auto request = DecodeActivateRequest(body, session_manager_, request_handle);
    if (!request.has_value()) {
      co_return std::nullopt;
    }
    auto response =
        co_await runtime_.ActivateSession(connection_, std::move(*request));
    co_return EncodeActivateSessionResponseBody(request_handle, response);
  }

  if (type_id == kCloseSessionRequestBinaryEncodingId) {
    const auto header = DecodeCloseRequestHeader(body);
    if (!header.has_value()) {
      co_return std::nullopt;
    }
    const auto session = session_manager_.FindSession(header->authentication_token);
    if (!session.has_value()) {
      co_return EncodeCloseSessionResponseBody(
          header->request_handle,
          {.status = scada::StatusCode::Bad_SessionIsLoggedOff});
    }
    auto response = co_await runtime_.CloseSession(
        connection_,
        {.session_id = session->session_id,
         .authentication_token = header->authentication_token});
    co_return EncodeCloseSessionResponseBody(header->request_handle, response);
  }

  co_return std::nullopt;
}

}  // namespace opcua
