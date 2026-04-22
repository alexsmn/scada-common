#include "opcua/opcua_binary_session_service.h"
#include "opcua/opcua_binary_codec_utils.h"
#include "opcua/opcua_binary_service_codec.h"

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

struct DecodedCreateSessionResponse {
  std::uint32_t status_code = 0;
  scada::NodeId session_id;
  scada::NodeId authentication_token;
};

std::optional<DecodedCreateSessionResponse> DecodeCreateSessionResponseBody(
    const std::vector<char>& bytes) {
  binary::BinaryDecoder message_decoder{bytes};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kCreateSessionResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  std::uint32_t request_handle = 0;
  std::uint32_t status_code = 0;
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  scada::NodeId ignored_additional_header;
  std::int64_t ignored_timestamp = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(request_handle) || !decoder.Decode(status_code) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_additional_header) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(session_id) ||
      !decoder.Decode(authentication_token)) {
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
  binary::BinaryDecoder message_decoder{bytes};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != expected_type_id) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t status_code = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(status_code)) {
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
  OpcUaSessionManager session_manager_{{
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
      executor_,
      service_.HandleRequest(
          7,
          OpcUaBinaryCreateSessionRequest{
              .requested_timeout =
                  base::TimeDelta::FromMillisecondsD(45000)}));
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
      executor_,
      service_.HandleRequest(
          1,
          OpcUaBinaryCreateSessionRequest{
              .requested_timeout =
                  base::TimeDelta::FromMillisecondsD(45000)}));
  ASSERT_TRUE(created.has_value());
  const auto decoded_create = DecodeCreateSessionResponseBody(*created);
  ASSERT_TRUE(decoded_create.has_value());

  const auto activated = WaitAwaitable(
      executor_,
      service_.HandleRequest(
          {.authentication_token = decoded_create->authentication_token,
           .request_handle = 2},
          OpcUaBinaryActivateSessionRequest{
              .user_name = scada::ToLocalizedText("operator"),
              .password = scada::ToLocalizedText("secret"),
              .allow_anonymous = false}));
  ASSERT_TRUE(activated.has_value());
  const auto activate_status =
      DecodeResponseStatus(*activated, kActivateSessionResponseBinaryEncodingId);
  ASSERT_TRUE(activate_status.has_value());
  EXPECT_EQ(*activate_status, 0u);

  const auto closed = WaitAwaitable(
      executor_,
      service_.HandleCloseRequest(3, decoded_create->authentication_token));
  ASSERT_TRUE(closed.has_value());
  const auto close_status =
      DecodeResponseStatus(*closed, kCloseSessionResponseBinaryEncodingId);
  ASSERT_TRUE(close_status.has_value());
  EXPECT_EQ(*close_status, 0u);
}

TEST_F(OpcUaBinarySessionServiceTest,
       ActivateSessionWithUnknownAuthenticationTokenReturnsNoPayload) {
  const auto activated = WaitAwaitable(
      executor_,
      service_.HandleRequest(
          {.authentication_token = scada::NodeId{999, 3}, .request_handle = 2},
          OpcUaBinaryActivateSessionRequest{
              .user_name = scada::ToLocalizedText("operator"),
              .password = scada::ToLocalizedText("secret"),
              .allow_anonymous = false}));
  EXPECT_FALSE(activated.has_value());
}

TEST_F(OpcUaBinarySessionServiceTest,
       CloseSessionWithUnknownAuthenticationTokenReturnsLoggedOff) {
  const auto closed = WaitAwaitable(
      executor_, service_.HandleCloseRequest(3, scada::NodeId{999, 3}));
  ASSERT_TRUE(closed.has_value());
  const auto close_status =
      DecodeResponseStatus(*closed, kCloseSessionResponseBinaryEncodingId);
  ASSERT_TRUE(close_status.has_value());
  EXPECT_EQ(*close_status,
            scada::Status(scada::StatusCode::Bad_SessionIsLoggedOff)
                .full_code());
}

}  // namespace
}  // namespace opcua
