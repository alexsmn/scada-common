#include "opcua/websocket/opcua_ws_runtime.h"

#include "opcua/opcua_runtime_contract_test.h"

#include <future>
#include <gtest/gtest.h>
#include <type_traits>

namespace opcua {
namespace {

class OpcUaWsRuntimeTest : public testing::Test,
                           public opcua::test::RuntimeContractTestBase {
 public:
  using ConnectionState = OpcUaWsConnectionState;

  static_assert(std::is_same_v<opcua::OpcUaRequestBody, opcua::OpcUaRequestBody>);
  static_assert(std::is_same_v<opcua::OpcUaResponseBody, opcua::OpcUaResponseBody>);

  template <typename Response, typename Request>
  Response HandleResponse(ConnectionState& connection, Request request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, opcua::OpcUaRequestBody{
                                                                 std::move(request)}));
    if (const auto* typed = std::get_if<Response>(&body))
      return *typed;
    ADD_FAILURE() << "unexpected response type";
    return {};
  }

  std::pair<scada::NodeId, scada::NodeId> CreateAndActivate(
      ConnectionState& connection) {
    const auto created =
        HandleResponse<opcua::OpcUaCreateSessionResponse>(
            connection, opcua::OpcUaCreateSessionRequest{});
    EXPECT_EQ(created.status.code(), scada::StatusCode::Good);

    const auto activated = HandleResponse<opcua::OpcUaActivateSessionResponse>(
        connection,
        opcua::OpcUaActivateSessionRequest{
            .session_id = created.session_id,
            .authentication_token = created.authentication_token,
            .user_name = scada::LocalizedText{u"operator"},
            .password = scada::LocalizedText{u"secret"},
        });
    EXPECT_EQ(activated.status.code(), scada::StatusCode::Good);
    EXPECT_FALSE(activated.resumed);
    return {created.session_id, created.authentication_token};
  }

  void Detach(ConnectionState& connection) { runtime_.Detach(connection); }

  scada::StatusCode ReadStatus(ConnectionState& connection, ReadRequest request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, opcua::OpcUaRequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<ReadResponse>(&body))
      return response->status.code();
    if (const auto* fault = std::get_if<opcua::OpcUaServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  scada::StatusCode HistoryReadRawStatus(ConnectionState& connection,
                                         HistoryReadRawRequest request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, opcua::OpcUaRequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<HistoryReadRawResponse>(&body))
      return response->result.status.code();
    if (const auto* fault = std::get_if<opcua::OpcUaServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  scada::StatusCode HistoryReadEventsStatus(ConnectionState& connection,
                                            HistoryReadEventsRequest request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, opcua::OpcUaRequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<HistoryReadEventsResponse>(&body))
      return response->result.status.code();
    if (const auto* fault = std::get_if<opcua::OpcUaServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  OpcUaWsRuntime runtime_{OpcUaWsRuntimeContext{
      .executor = any_executor_,
      .session_manager = session_manager_,
      .monitored_item_service = monitored_item_service_,
      .attribute_service = attribute_service_,
      .view_service = view_service_,
      .history_service = history_service_,
      .method_service = method_service_,
      .node_management_service = node_management_service_,
      .now = [this] { return now_; },
      .post_delayed_task = [this](base::TimeDelta d, std::function<void()> fn) {
        executor_->PostDelayedTask(
            std::chrono::milliseconds{d.InMilliseconds()}, std::move(fn));
      },
  }};
};

TEST_F(OpcUaWsRuntimeTest, RoutesReadRequestsThroughActivatedSessionUser) {
  opcua::test::ExpectRoutesReadRequestsThroughActivatedSessionUser(*this);
}

TEST_F(OpcUaWsRuntimeTest, RoutesWriteRequestsThroughActivatedSessionUser) {
  opcua::test::ExpectRoutesWriteRequestsThroughActivatedSessionUser(*this);
}

TEST_F(OpcUaWsRuntimeTest, RoutesCallRequestsThroughActivatedSessionUser) {
  opcua::test::ExpectRoutesCallRequestsThroughActivatedSessionUser(*this);
}

TEST_F(OpcUaWsRuntimeTest, PreservesLiveSubscriptionStateAcrossDetachAndResume) {
  opcua::test::ExpectPreservesLiveSubscriptionStateAcrossDetachAndResume(*this);
}

TEST_F(OpcUaWsRuntimeTest, TransfersSubscriptionsAcrossSessions) {
  opcua::test::ExpectTransfersSubscriptionsAcrossSessions(*this);
}

TEST_F(OpcUaWsRuntimeTest, CloseSessionClearsAttachedState) {
  opcua::test::ExpectCloseSessionClearsAttachedState(*this);
}

TEST_F(OpcUaWsRuntimeTest, RejectsHistoryReadRawWithoutActivatedSession) {
  opcua::test::ExpectRejectsHistoryReadRawWithoutActivatedSession(*this);
}

TEST_F(OpcUaWsRuntimeTest,
       HistoryReadRawPreservesPayloadThroughActivatedSession) {
  opcua::test::ExpectHistoryReadRawPreservesPayloadThroughActivatedSession(*this);
}

TEST_F(OpcUaWsRuntimeTest, RejectsHistoryReadEventsWithoutActivatedSession) {
  opcua::test::ExpectRejectsHistoryReadEventsWithoutActivatedSession(*this);
}

TEST_F(OpcUaWsRuntimeTest,
       HistoryReadEventsPreservesPayloadThroughActivatedSession) {
  opcua::test::ExpectHistoryReadEventsPreservesPayloadThroughActivatedSession(
      *this);
}

TEST_F(OpcUaWsRuntimeTest, BrowseAndBrowseNextUseSessionScopedContinuationPoints) {
  opcua::test::ExpectBrowseAndBrowseNextUseSessionScopedContinuationPoints(*this);
}

TEST_F(OpcUaWsRuntimeTest, NodeManagementMutationsPreserveBatchResults) {
  opcua::test::ExpectNodeManagementMutationsPreserveBatchResults(*this);
}

TEST_F(OpcUaWsRuntimeTest, PublishReturnsKeepAliveWhenNoNotificationsAreQueued) {
  opcua::test::ExpectPublishReturnsKeepAliveWhenNoNotifications(*this);
}

TEST_F(OpcUaWsRuntimeTest, RepublishReplaysNotificationUntilAcknowledged) {
  opcua::test::ExpectRepublishReplaysNotificationUntilAcknowledged(*this);
}

TEST_F(OpcUaWsRuntimeTest, PublishRequestWaitsForKeepAliveDeadline) {
  OpcUaWsConnectionState connection;
  CreateAndActivate(connection);

  const auto created_subscription =
      HandleResponse<opcua::OpcUaCreateSubscriptionResponse>(
          connection,
          opcua::OpcUaCreateSubscriptionRequest{
              .parameters = {.publishing_interval_ms = 100,
                             .lifetime_count = 60,
                             .max_keep_alive_count = 3,
                             .publishing_enabled = true}});
  EXPECT_EQ(created_subscription.status.code(), scada::StatusCode::Good);

  promise<opcua::OpcUaResponseBody> publish_promise;
  CoSpawn(MakeTestAnyExecutor(executor_),
          [this, &connection, &publish_promise]() mutable -> Awaitable<void> {
            try {
              publish_promise.resolve(
                  co_await runtime_.Handle(connection,
                                           opcua::OpcUaRequestBody{
                                               opcua::OpcUaPublishRequest{}}));
            } catch (...) {
              publish_promise.reject(std::current_exception());
            }
          });

  const auto drain_ready = [&] {
    for (size_t i = 0; i < 8; ++i)
      executor_->Poll();
  };

  drain_ready();
  EXPECT_EQ(publish_promise.wait_for(0ms), promise_wait_status::timeout);

  now_ = now_ + base::TimeDelta::FromMilliseconds(300);
  executor_->Advance(300ms);
  drain_ready();
  ASSERT_NE(publish_promise.wait_for(0ms), promise_wait_status::timeout);

  const auto publish_message = publish_promise.get();
  const auto* publish =
      std::get_if<opcua::OpcUaPublishResponse>(&publish_message);
  ASSERT_NE(publish, nullptr);
  EXPECT_EQ(publish->status.code(), scada::StatusCode::Good);
  EXPECT_TRUE(publish->notification_message.notification_data.empty());
}

}  // namespace
}  // namespace opcua
