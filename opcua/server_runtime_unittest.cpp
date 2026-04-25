#include "opcua/server_runtime.h"

#include "opcua/server_runtime_contract_test.h"

#include <gtest/gtest.h>

namespace opcua {
namespace {

class ServerRuntimeTest
    : public testing::Test,
      public test::ServerRuntimeContractTestBase {
 public:
  using ConnectionState = opcua::ConnectionState;

  template <typename Response, typename Request>
  Response HandleResponse(ConnectionState& connection, Request request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, RequestBody{
                                                                 std::move(request)}));
    if (const auto* typed = std::get_if<Response>(&body))
      return *typed;
    ADD_FAILURE() << "unexpected response type";
    return {};
  }

  std::pair<scada::NodeId, scada::NodeId> CreateAndActivate(
      ConnectionState& connection) {
    const auto created =
        HandleResponse<CreateSessionResponse>(connection,
                                                   CreateSessionRequest{});
    EXPECT_EQ(created.status.code(), scada::StatusCode::Good);

    const auto activated = HandleResponse<ActivateSessionResponse>(
        connection,
        ActivateSessionRequest{
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
        WaitAwaitable(executor_, runtime_.Handle(connection, RequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<ReadResponse>(&body))
      return response->status.code();
    if (const auto* fault = std::get_if<ServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  scada::StatusCode HistoryReadRawStatus(ConnectionState& connection,
                                         HistoryReadRawRequest request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, RequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<HistoryReadRawResponse>(&body))
      return response->result.status.code();
    if (const auto* fault = std::get_if<ServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  scada::StatusCode HistoryReadEventsStatus(ConnectionState& connection,
                                            HistoryReadEventsRequest request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, RequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<HistoryReadEventsResponse>(&body))
      return response->result.status.code();
    if (const auto* fault = std::get_if<ServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  ServerRuntime runtime_{ServerRuntimeContext{
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

TEST_F(ServerRuntimeTest, RoutesReadRequestsThroughActivatedSessionUser) {
  test::ExpectRoutesReadRequestsThroughActivatedSessionUser(*this);
}

TEST_F(ServerRuntimeTest, RoutesWriteRequestsThroughActivatedSessionUser) {
  test::ExpectRoutesWriteRequestsThroughActivatedSessionUser(*this);
}

TEST_F(ServerRuntimeTest, RoutesCallRequestsThroughActivatedSessionUser) {
  test::ExpectRoutesCallRequestsThroughActivatedSessionUser(*this);
}

TEST_F(ServerRuntimeTest, PreservesLiveSubscriptionStateAcrossDetachAndResume) {
  test::ExpectPreservesLiveSubscriptionStateAcrossDetachAndResume(*this);
}

TEST_F(ServerRuntimeTest, TransfersSubscriptionsAcrossSessions) {
  test::ExpectTransfersSubscriptionsAcrossSessions(*this);
}

TEST_F(ServerRuntimeTest, CloseSessionClearsAttachedState) {
  test::ExpectCloseSessionClearsAttachedState(*this);
}

TEST_F(ServerRuntimeTest, RejectsHistoryReadRawWithoutActivatedSession) {
  test::ExpectRejectsHistoryReadRawWithoutActivatedSession(*this);
}

TEST_F(ServerRuntimeTest, HistoryReadRawPreservesPayloadThroughActivatedSession) {
  test::ExpectHistoryReadRawPreservesPayloadThroughActivatedSession(*this);
}

TEST_F(ServerRuntimeTest, RejectsHistoryReadEventsWithoutActivatedSession) {
  test::ExpectRejectsHistoryReadEventsWithoutActivatedSession(*this);
}

TEST_F(ServerRuntimeTest,
       HistoryReadEventsPreservesPayloadThroughActivatedSession) {
  test::ExpectHistoryReadEventsPreservesPayloadThroughActivatedSession(
      *this);
}

TEST_F(ServerRuntimeTest, BrowseAndBrowseNextUseSessionScopedContinuationPoints) {
  test::ExpectBrowseAndBrowseNextUseSessionScopedContinuationPoints(*this);
}

TEST_F(ServerRuntimeTest, NodeManagementMutationsPreserveBatchResults) {
  test::ExpectNodeManagementMutationsPreserveBatchResults(*this);
}

TEST_F(ServerRuntimeTest, PublishReturnsKeepAliveWhenNoNotificationsAreQueued) {
  test::ExpectPublishReturnsKeepAliveWhenNoNotifications(*this);
}

TEST_F(ServerRuntimeTest, RepublishReplaysNotificationUntilAcknowledged) {
  test::ExpectRepublishReplaysNotificationUntilAcknowledged(*this);
}

}  // namespace
}  // namespace opcua
