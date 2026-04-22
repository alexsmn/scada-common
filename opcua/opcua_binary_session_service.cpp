#include "opcua/opcua_binary_session_service.h"
#include "opcua/opcua_binary_codec_utils.h"

#include "scada/localized_text.h"

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

using binary::AppendByteString;
using binary::AppendDouble;
using binary::AppendInt32;
using binary::AppendInt64;
using binary::AppendMessage;
using binary::AppendNumericNodeId;
using binary::AppendUaString;
using binary::AppendUInt8;
using binary::AppendUInt16;
using binary::AppendUInt32;
using binary::ReadByteString;
using binary::ReadDouble;
using binary::ReadExtensionObject;
using binary::ReadInt32;
using binary::ReadInt64;
using binary::ReadMessage;
using binary::ReadNumericNodeId;
using binary::ReadUaString;
using binary::ReadUInt8;
using binary::ReadUInt16;
using binary::ReadUInt32;

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
