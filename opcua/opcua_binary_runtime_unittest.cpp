#include "opcua/opcua_binary_runtime.h"

#include "opcua/opcua_runtime_contract_test.h"

#include <gtest/gtest.h>

namespace opcua {
namespace {

static_assert(std::is_same_v<OpcUaBinaryRequestBody, OpcUaRequestBody>);
static_assert(std::is_same_v<OpcUaBinaryResponseBody, OpcUaResponseBody>);
static_assert(
    std::is_same_v<OpcUaBinaryHistoryReadEventsRequest, HistoryReadEventsRequest>);
static_assert(std::is_same_v<OpcUaBinaryHistoryReadEventsResponse,
                             HistoryReadEventsResponse>);

class OpcUaBinaryRuntimeTest : public testing::Test,
                               public opcua::test::RuntimeContractTestBase {
 public:
  using ConnectionState = OpcUaBinaryConnectionState;

  template <typename Response, typename Request>
  Response HandleResponse(ConnectionState& connection, Request request) {
    return WaitAwaitable(executor_,
                         runtime_.Handle<Response>(connection,
                                                   std::move(request)));
  }

  std::pair<scada::NodeId, scada::NodeId> CreateAndActivate(
      ConnectionState& connection) {
    const auto created = HandleResponse<OpcUaBinaryCreateSessionResponse>(
        connection, OpcUaBinaryCreateSessionRequest{});
    EXPECT_EQ(created.status.code(), scada::StatusCode::Good);

    const auto activated = HandleResponse<OpcUaBinaryActivateSessionResponse>(
        connection,
        OpcUaBinaryActivateSessionRequest{
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
    return HandleResponse<OpcUaBinaryReadResponse>(connection, std::move(request))
        .status.code();
  }

  scada::StatusCode HistoryReadRawStatus(ConnectionState& connection,
                                         HistoryReadRawRequest request) {
    return HandleResponse<OpcUaBinaryHistoryReadRawResponse>(
               connection, std::move(request))
        .result.status.code();
  }

  scada::StatusCode HistoryReadEventsStatus(ConnectionState& connection,
                                            HistoryReadEventsRequest request) {
    return HandleResponse<OpcUaBinaryHistoryReadEventsResponse>(
               connection, std::move(request))
        .result.status.code();
  }

  OpcUaBinaryRuntime runtime_{OpcUaBinaryRuntimeContext{
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

TEST_F(OpcUaBinaryRuntimeTest, RoutesReadRequestsThroughActivatedSessionUser) {
  opcua::test::ExpectRoutesReadRequestsThroughActivatedSessionUser(*this);
}

TEST_F(OpcUaBinaryRuntimeTest,
       PreservesLiveSubscriptionStateAcrossDetachAndResume) {
  opcua::test::ExpectPreservesLiveSubscriptionStateAcrossDetachAndResume(*this);
}

TEST_F(OpcUaBinaryRuntimeTest, TransfersSubscriptionsAcrossSessions) {
  opcua::test::ExpectTransfersSubscriptionsAcrossSessions(*this);
}

TEST_F(OpcUaBinaryRuntimeTest, CloseSessionClearsAttachedState) {
  opcua::test::ExpectCloseSessionClearsAttachedState(*this);
}

TEST_F(OpcUaBinaryRuntimeTest,
       RejectsHistoryReadRawWithoutActivatedSession) {
  opcua::test::ExpectRejectsHistoryReadRawWithoutActivatedSession(*this);
}

TEST_F(OpcUaBinaryRuntimeTest,
       RejectsHistoryReadEventsWithoutActivatedSession) {
  opcua::test::ExpectRejectsHistoryReadEventsWithoutActivatedSession(*this);
}

}  // namespace
}  // namespace opcua
