#include "opcua/opcua_runtime.h"

#include "opcua/opcua_runtime_contract_test.h"

#include <gtest/gtest.h>

namespace opcua {
namespace {

class OpcUaRuntimeTest : public testing::Test,
                         public opcua::test::RuntimeContractTestBase {
 public:
  using ConnectionState = OpcUaConnectionState;

  template <typename Response, typename Request>
  Response HandleResponse(ConnectionState& connection, Request request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, OpcUaRequestBody{
                                                                 std::move(request)}));
    if (const auto* typed = std::get_if<Response>(&body))
      return *typed;
    ADD_FAILURE() << "unexpected response type";
    return {};
  }

  std::pair<scada::NodeId, scada::NodeId> CreateAndActivate(
      ConnectionState& connection) {
    const auto created =
        HandleResponse<OpcUaCreateSessionResponse>(connection,
                                                   OpcUaCreateSessionRequest{});
    EXPECT_EQ(created.status.code(), scada::StatusCode::Good);

    const auto activated = HandleResponse<OpcUaActivateSessionResponse>(
        connection,
        OpcUaActivateSessionRequest{
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
        WaitAwaitable(executor_, runtime_.Handle(connection, OpcUaRequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<ReadResponse>(&body))
      return response->status.code();
    if (const auto* fault = std::get_if<OpcUaServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  scada::StatusCode HistoryReadRawStatus(ConnectionState& connection,
                                         HistoryReadRawRequest request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, OpcUaRequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<HistoryReadRawResponse>(&body))
      return response->result.status.code();
    if (const auto* fault = std::get_if<OpcUaServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  scada::StatusCode HistoryReadEventsStatus(ConnectionState& connection,
                                            HistoryReadEventsRequest request) {
    const auto body =
        WaitAwaitable(executor_, runtime_.Handle(connection, OpcUaRequestBody{
                                                                 std::move(request)}));
    if (const auto* response = std::get_if<HistoryReadEventsResponse>(&body))
      return response->result.status.code();
    if (const auto* fault = std::get_if<OpcUaServiceFault>(&body))
      return fault->status.code();
    return scada::StatusCode::Bad;
  }

  OpcUaRuntime runtime_{OpcUaRuntimeContext{
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

TEST_F(OpcUaRuntimeTest, RoutesReadRequestsThroughActivatedSessionUser) {
  opcua::test::ExpectRoutesReadRequestsThroughActivatedSessionUser(*this);
}

TEST_F(OpcUaRuntimeTest, PreservesLiveSubscriptionStateAcrossDetachAndResume) {
  opcua::test::ExpectPreservesLiveSubscriptionStateAcrossDetachAndResume(*this);
}

TEST_F(OpcUaRuntimeTest, TransfersSubscriptionsAcrossSessions) {
  opcua::test::ExpectTransfersSubscriptionsAcrossSessions(*this);
}

TEST_F(OpcUaRuntimeTest, CloseSessionClearsAttachedState) {
  opcua::test::ExpectCloseSessionClearsAttachedState(*this);
}

TEST_F(OpcUaRuntimeTest, RejectsHistoryReadRawWithoutActivatedSession) {
  opcua::test::ExpectRejectsHistoryReadRawWithoutActivatedSession(*this);
}

TEST_F(OpcUaRuntimeTest, RejectsHistoryReadEventsWithoutActivatedSession) {
  opcua::test::ExpectRejectsHistoryReadEventsWithoutActivatedSession(*this);
}

}  // namespace
}  // namespace opcua
