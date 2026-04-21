#include "opcua/opcua_binary_session_service.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "base/time_utils.h"
#include "scada/attribute_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/node_management_service_mock.h"
#include "scada/view_service_mock.h"

#include <gtest/gtest.h>

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

void AppendUaString(std::vector<char>& bytes, std::string_view value) {
  AppendInt32(bytes, static_cast<std::int32_t>(value.size()));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

void AppendByteString(std::vector<char>& bytes, const scada::ByteString& value) {
  AppendInt32(bytes, static_cast<std::int32_t>(value.size()));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

void AppendNumericNodeId(std::vector<char>& bytes, const scada::NodeId& node_id) {
  if (node_id.is_null()) {
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

void AppendExtensionObject(std::vector<char>& bytes,
                           std::uint32_t type_id,
                           const std::vector<char>& body) {
  AppendNumericNodeId(bytes, scada::NodeId{type_id});
  AppendUInt8(bytes, 0x01);
  AppendInt32(bytes, static_cast<std::int32_t>(body.size()));
  bytes.insert(bytes.end(), body.begin(), body.end());
}

void AppendMessage(std::vector<char>& bytes,
                   std::uint32_t type_id,
                   const std::vector<char>& body) {
  AppendNumericNodeId(bytes, scada::NodeId{type_id});
  bytes.insert(bytes.end(), body.begin(), body.end());
}

void AppendRequestHeader(std::vector<char>& bytes,
                         const scada::NodeId& authentication_token,
                         std::uint32_t request_handle) {
  AppendNumericNodeId(bytes, authentication_token);
  AppendInt64(bytes, 0);
  AppendUInt32(bytes, request_handle);
  AppendUInt32(bytes, 0);
  AppendUaString(bytes, "");
  AppendUInt32(bytes, 0);
  AppendNumericNodeId(bytes, scada::NodeId{});
  AppendUInt8(bytes, 0x00);
}

std::vector<char> EncodeCreateSessionRequestBody(std::uint32_t request_handle,
                                                 double requested_timeout_ms) {
  std::vector<char> payload;
  AppendRequestHeader(payload, scada::NodeId{}, request_handle);
  AppendUaString(payload, "");
  AppendUaString(payload, "");
  AppendUInt8(payload, 0);
  AppendInt32(payload, 0);
  AppendUaString(payload, "");
  AppendUaString(payload, "");
  AppendInt32(payload, -1);
  AppendUaString(payload, "");
  AppendUaString(payload, "opc.tcp://localhost:4840");
  AppendUaString(payload, "binary-session");
  AppendByteString(payload, {});
  AppendByteString(payload, {});
  AppendDouble(payload, requested_timeout_ms);
  AppendUInt32(payload, 0);

  std::vector<char> body;
  AppendMessage(body, kCreateSessionRequestBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeAnonymousActivateRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token) {
  std::vector<char> payload;
  AppendRequestHeader(payload, authentication_token, request_handle);
  AppendUaString(payload, "");
  AppendByteString(payload, {});
  AppendInt32(payload, -1);
  AppendInt32(payload, -1);
  std::vector<char> identity;
  AppendUaString(identity, "");
  AppendExtensionObject(payload, kAnonymousIdentityTokenBinaryEncodingId,
                        identity);
  AppendUaString(payload, "");
  AppendByteString(payload, {});

  std::vector<char> body;
  AppendMessage(body, kActivateSessionRequestBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeUserNameActivateRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    std::string_view user_name,
    std::string_view password) {
  std::vector<char> payload;
  AppendRequestHeader(payload, authentication_token, request_handle);
  AppendUaString(payload, "");
  AppendByteString(payload, {});
  AppendInt32(payload, -1);
  AppendInt32(payload, -1);
  std::vector<char> identity;
  AppendUaString(identity, "");
  AppendUaString(identity, user_name);
  AppendByteString(identity, scada::ByteString{password.begin(), password.end()});
  AppendUaString(identity, "");
  AppendExtensionObject(payload, kUserNameIdentityTokenBinaryEncodingId,
                        identity);
  AppendUaString(payload, "");
  AppendByteString(payload, {});

  std::vector<char> body;
  AppendMessage(body, kActivateSessionRequestBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeCloseSessionRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token) {
  std::vector<char> payload;
  AppendRequestHeader(payload, authentication_token, request_handle);
  AppendUInt8(payload, 1);

  std::vector<char> body;
  AppendMessage(body, kCloseSessionRequestBinaryEncodingId, payload);
  return body;
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

bool ReadExtensionObject(const std::vector<char>& bytes,
                         std::size_t& offset,
                         std::uint32_t& type_id,
                         std::vector<char>& body) {
  scada::NodeId type_node_id;
  std::uint8_t encoding = 0;
  if (!ReadNumericNodeId(bytes, offset, type_node_id) ||
      !type_node_id.is_numeric() ||
      !ReadUInt8(bytes, offset, encoding) || encoding != 0x01) {
    return false;
  }
  type_id = type_node_id.numeric_id();
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

struct DecodedCreateSessionResponse {
  std::uint32_t status_code = 0;
  scada::NodeId session_id;
  scada::NodeId authentication_token;
};

std::optional<DecodedCreateSessionResponse> DecodeCreateSessionResponseBody(
    const std::vector<char>& bytes) {
  std::size_t offset = 0;
  scada::NodeId type_node_id;
  if (!ReadNumericNodeId(bytes, offset, type_node_id) ||
      !type_node_id.is_numeric() ||
      type_node_id.numeric_id() != kCreateSessionResponseBinaryEncodingId) {
    return std::nullopt;
  }
  std::vector<char> body{bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                         bytes.end()};
  std::size_t body_offset = 0;
  std::uint32_t request_handle = 0;
  std::uint32_t status_code = 0;
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  scada::NodeId ignored_additional_header;
  std::int64_t ignored_timestamp = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  if (!ReadInt64(body, body_offset, ignored_timestamp) ||
      !ReadUInt32(body, body_offset, request_handle) ||
      !ReadUInt32(body, body_offset, status_code) ||
      !ReadUInt8(body, body_offset, ignored_byte) ||
      !ReadInt32(body, body_offset, ignored_array) ||
      !ReadNumericNodeId(body, body_offset, ignored_additional_header) ||
      !ReadUInt8(body, body_offset, ignored_byte) ||
      !ReadNumericNodeId(body, body_offset, session_id) ||
      !ReadNumericNodeId(body, body_offset, authentication_token)) {
    return std::nullopt;
  }
  return DecodedCreateSessionResponse{
      .status_code = status_code,
      .session_id = session_id,
      .authentication_token = authentication_token,
  };
}

std::optional<std::uint32_t> DecodeResponseStatus(const std::vector<char>& bytes,
                                                  std::uint32_t expected_type_id) {
  std::size_t offset = 0;
  scada::NodeId type_node_id;
  if (!ReadNumericNodeId(bytes, offset, type_node_id) ||
      !type_node_id.is_numeric() ||
      type_node_id.numeric_id() != expected_type_id) {
    return std::nullopt;
  }
  std::vector<char> body{bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                         bytes.end()};
  std::size_t body_offset = 0;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t status_code = 0;
  if (!ReadInt64(body, body_offset, ignored_timestamp) ||
      !ReadUInt32(body, body_offset, ignored_request_handle) ||
      !ReadUInt32(body, body_offset, status_code)) {
    return std::nullopt;
  }
  return status_code;
}

base::Time ParseTime(std::string_view value) {
  base::Time result;
  EXPECT_TRUE(Deserialize(value, result));
  return result;
}

class TestMonitoredItemService : public scada::MonitoredItemService {
 public:
  std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId&,
      const scada::MonitoringParameters&) override {
    return nullptr;
  }
};

class OpcUaBinarySessionServiceTest : public ::testing::Test {
 protected:
  base::Time now_ = ParseTime("2026-04-21 10:00:00");
  const scada::NodeId expected_user_id_{700, 5};
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  scada::MockAttributeService attribute_service_;
  scada::MockViewService view_service_;
  scada::MockHistoryService history_service_;
  scada::MockMethodService method_service_;
  scada::MockNodeManagementService node_management_service_;
  TestMonitoredItemService monitored_item_service_;
  opcua_ws::OpcUaWsSessionManager session_manager_{{
      .authenticator =
          [this](scada::LocalizedText user_name,
                 scada::LocalizedText password)
              -> Awaitable<scada::StatusOr<scada::AuthenticationResult>> {
        EXPECT_EQ(user_name, scada::LocalizedText{u"operator"});
        EXPECT_EQ(password, scada::LocalizedText{u"secret"});
        co_return scada::AuthenticationResult{
            .user_id = expected_user_id_, .multi_sessions = true};
      },
      .now = [this] { return now_; },
  }};
  OpcUaBinaryConnectionState connection_;
  OpcUaBinaryRuntime runtime_{{
      .executor = executor_,
      .session_manager = session_manager_,
      .monitored_item_service = monitored_item_service_,
      .attribute_service = attribute_service_,
      .view_service = view_service_,
      .history_service = history_service_,
      .method_service = method_service_,
      .node_management_service = node_management_service_,
      .now = [this] { return now_; },
  }};
  OpcUaBinarySessionService service_{
      {.runtime = runtime_,
       .session_manager = session_manager_,
       .connection = connection_}};
};

TEST_F(OpcUaBinarySessionServiceTest, HandlesCreateSessionOverBinaryPayload) {
  const auto response = WaitAwaitable(
      executor_, service_.HandlePayload(EncodeCreateSessionRequestBody(7, 45000)));
  ASSERT_TRUE(response.has_value());

  const auto decoded = DecodeCreateSessionResponseBody(*response);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->status_code, 0u);
  EXPECT_TRUE(decoded->session_id.is_numeric());
  EXPECT_TRUE(decoded->authentication_token.is_numeric());
  EXPECT_EQ(decoded->authentication_token.namespace_index(), 3u);
}

TEST_F(OpcUaBinarySessionServiceTest,
       ActivatesAndClosesSessionOverBinaryPayload) {
  const auto created = WaitAwaitable(
      executor_, service_.HandlePayload(EncodeCreateSessionRequestBody(1, 45000)));
  ASSERT_TRUE(created.has_value());
  const auto decoded_create = DecodeCreateSessionResponseBody(*created);
  ASSERT_TRUE(decoded_create.has_value());

  const auto activated = WaitAwaitable(
      executor_,
      service_.HandlePayload(EncodeUserNameActivateRequestBody(
          2, decoded_create->authentication_token, "operator", "secret")));
  ASSERT_TRUE(activated.has_value());
  const auto activate_status =
      DecodeResponseStatus(*activated, kActivateSessionResponseBinaryEncodingId);
  ASSERT_TRUE(activate_status.has_value());
  EXPECT_EQ(*activate_status, 0u);

  const auto closed = WaitAwaitable(
      executor_,
      service_.HandlePayload(
          EncodeCloseSessionRequestBody(3, decoded_create->authentication_token)));
  ASSERT_TRUE(closed.has_value());
  const auto close_status =
      DecodeResponseStatus(*closed, kCloseSessionResponseBinaryEncodingId);
  ASSERT_TRUE(close_status.has_value());
  EXPECT_EQ(*close_status, 0u);
}

}  // namespace
}  // namespace opcua
