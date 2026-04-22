#include "opcua_ws/opcua_ws_runtime.h"

#include "opcua/opcua_runtime_contract_test.h"

#include <future>
#include <gtest/gtest.h>

namespace opcua_ws {
namespace {

class OpcUaWsRuntimeTest : public testing::Test,
                           public opcua::test::RuntimeContractTestBase {
 public:
  using ConnectionState = OpcUaWsConnectionState;

  template <typename Response, typename Request>
  Response HandleResponse(ConnectionState& connection, Request request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, OpcUaWsRequestBody{
                                                                 std::move(request)}));
    if (const auto* typed = std::get_if<Response>(&body))
      return *typed;
    ADD_FAILURE() << "unexpected response type";
    return {};
  }

  std::pair<scada::NodeId, scada::NodeId> CreateAndActivate(
      ConnectionState& connection) {
    const auto created =
        HandleResponse<OpcUaWsCreateSessionResponse>(connection,
                                                     OpcUaWsCreateSessionRequest{});
    EXPECT_EQ(created.status.code(), scada::StatusCode::Good);

    const auto activated = HandleResponse<OpcUaWsActivateSessionResponse>(
        connection,
        OpcUaWsActivateSessionRequest{
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
        WaitAwaitable(executor_, runtime_.Handle(connection, OpcUaWsRequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<ReadResponse>(&body))
      return response->status.code();
    if (const auto* fault = std::get_if<OpcUaWsServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  scada::StatusCode HistoryReadRawStatus(ConnectionState& connection,
                                         HistoryReadRawRequest request) {
    return HandleResponse<HistoryReadRawResponse>(connection, std::move(request))
        .result.status.code();
  }

  scada::StatusCode HistoryReadEventsStatus(ConnectionState& connection,
                                            HistoryReadEventsRequest request) {
    return HandleResponse<HistoryReadEventsResponse>(
               connection, std::move(request))
        .result.status.code();
  }

  OpcUaWsRuntime runtime_{OpcUaWsRuntimeContext{
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
};

TEST_F(OpcUaWsRuntimeTest, RoutesReadRequestsThroughActivatedSessionUser) {
  opcua::test::ExpectRoutesReadRequestsThroughActivatedSessionUser(*this);
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

TEST_F(OpcUaWsRuntimeTest, RejectsHistoryReadEventsWithoutActivatedSession) {
  opcua::test::ExpectRejectsHistoryReadEventsWithoutActivatedSession(*this);
}

TEST_F(OpcUaWsRuntimeTest, BrowseAndBrowseNextUseSessionScopedContinuationPoints) {
  OpcUaWsConnectionState connection;
  CreateAndActivate(connection);

  EXPECT_CALL(view_service_, Browse(testing::_, testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [&](const scada::ServiceContext& context,
              const std::vector<scada::BrowseDescription>& inputs,
              const scada::BrowseCallback& callback) {
            EXPECT_EQ(context.user_id(), expected_user_id_);
            ASSERT_EQ(inputs.size(), 1u);
            callback(scada::StatusCode::Good,
                     {scada::BrowseResult{
                         .status_code = scada::StatusCode::Good,
                         .references = {
                             {.reference_type_id = opcua::test::NumericNode(901),
                              .forward = true,
                              .node_id = opcua::test::NumericNode(902)},
                             {.reference_type_id = opcua::test::NumericNode(903),
                              .forward = false,
                              .node_id = opcua::test::NumericNode(904)},
                             {.reference_type_id = opcua::test::NumericNode(905),
                              .forward = true,
                              .node_id = opcua::test::NumericNode(906)}}}});
          }));

  const auto browse = HandleResponse<BrowseResponse>(
      connection,
      BrowseRequest{
          .requested_max_references_per_node = 2,
          .inputs = {{.node_id = opcua::test::NumericNode(900),
                      .direction = scada::BrowseDirection::Both,
                      .reference_type_id = opcua::test::NumericNode(910),
                      .include_subtypes = true}}});
  ASSERT_EQ(browse.results.size(), 1u);
  ASSERT_EQ(browse.results[0].references.size(), 2u);
  ASSERT_FALSE(browse.results[0].continuation_point.empty());
  EXPECT_EQ(browse.results[0].references[0].node_id,
            opcua::test::NumericNode(902));
  EXPECT_EQ(browse.results[0].references[1].node_id,
            opcua::test::NumericNode(904));

  const auto browse_next = HandleResponse<BrowseNextResponse>(
      connection,
      BrowseNextRequest{
          .continuation_points = {browse.results[0].continuation_point}});
  ASSERT_EQ(browse_next.results.size(), 1u);
  EXPECT_EQ(browse_next.results[0].status_code, scada::StatusCode::Good);
  ASSERT_EQ(browse_next.results[0].references.size(), 1u);
  EXPECT_EQ(browse_next.results[0].references[0].node_id,
            opcua::test::NumericNode(906));

  const auto invalid = HandleResponse<BrowseNextResponse>(
      connection,
      BrowseNextRequest{
          .continuation_points = {browse.results[0].continuation_point}});
  ASSERT_EQ(invalid.results.size(), 1u);
  EXPECT_EQ(invalid.results[0].status_code,
            scada::StatusCode::Bad_WrongIndex);
}

TEST_F(OpcUaWsRuntimeTest, PublishRequestWaitsForKeepAliveDeadline) {
  OpcUaWsConnectionState connection;
  CreateAndActivate(connection);

  const auto created_subscription =
      HandleResponse<OpcUaWsCreateSubscriptionResponse>(
          connection,
          OpcUaWsCreateSubscriptionRequest{
              .parameters = {.publishing_interval_ms = 100,
                             .lifetime_count = 60,
                             .max_keep_alive_count = 3,
                             .publishing_enabled = true}});
  EXPECT_EQ(created_subscription.status.code(), scada::StatusCode::Good);

  promise<OpcUaWsResponseBody> publish_promise;
  CoSpawn(MakeTestAnyExecutor(executor_),
          [this, &connection, &publish_promise]() mutable -> Awaitable<void> {
            try {
              publish_promise.resolve(
                  co_await runtime_.Handle(connection,
                                           OpcUaWsRequestBody{
                                               OpcUaWsPublishRequest{}}));
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
  const auto* publish = std::get_if<OpcUaWsPublishResponse>(&publish_message);
  ASSERT_NE(publish, nullptr);
  EXPECT_EQ(publish->status.code(), scada::StatusCode::Good);
  EXPECT_TRUE(publish->notification_message.notification_data.empty());
}

}  // namespace
}  // namespace opcua_ws
