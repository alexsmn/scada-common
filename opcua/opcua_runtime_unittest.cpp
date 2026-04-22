#include "opcua/opcua_runtime.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "base/time_utils.h"
#include "scada/attribute_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/node_management_service_mock.h"
#include "scada/view_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace opcua {
namespace {

scada::NodeId NumericNode(scada::NumericId id, scada::NamespaceIndex ns = 2) {
  return {id, ns};
}

base::Time ParseTime(std::string_view value) {
  base::Time result;
  EXPECT_TRUE(Deserialize(value, result));
  return result;
}

class NullMonitoredItemService : public scada::MonitoredItemService {
 public:
  std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId&,
      const scada::MonitoringParameters&) override {
    return nullptr;
  }
};

TEST(OpcUaRuntimeCanonicalTest, HandleRead_UsesCanonicalRuntimeSurface) {
  const auto now = ParseTime("2026-04-22 09:30:00");
  const auto expected_user_id = NumericNode(700, 5);
  const auto executor = std::make_shared<TestExecutor>();
  StrictMock<scada::MockAttributeService> attribute_service;
  StrictMock<scada::MockViewService> view_service;
  StrictMock<scada::MockHistoryService> history_service;
  StrictMock<scada::MockMethodService> method_service;
  StrictMock<scada::MockNodeManagementService> node_management_service;
  NullMonitoredItemService monitored_item_service;
  OpcUaSessionManager session_manager{{
      .authenticator =
          [&](scada::LocalizedText user_name,
              scada::LocalizedText password)
              -> Awaitable<scada::StatusOr<scada::AuthenticationResult>> {
        EXPECT_EQ(user_name, scada::LocalizedText{u"operator"});
        EXPECT_EQ(password, scada::LocalizedText{u"secret"});
        co_return scada::AuthenticationResult{
            .user_id = expected_user_id, .multi_sessions = true};
      },
      .now = [now] { return now; },
  }};
  OpcUaRuntime runtime{OpcUaRuntimeContext{
      .executor = executor,
      .session_manager = session_manager,
      .monitored_item_service = monitored_item_service,
      .attribute_service = attribute_service,
      .view_service = view_service,
      .history_service = history_service,
      .method_service = method_service,
      .node_management_service = node_management_service,
      .now = [now] { return now; },
  }};

  OpcUaConnectionState connection;
  const auto created = WaitAwaitable(
      executor, runtime.Handle(connection, OpcUaRequestBody{OpcUaCreateSessionRequest{}}));
  const auto* create_response = std::get_if<OpcUaCreateSessionResponse>(&created);
  ASSERT_NE(create_response, nullptr);
  ASSERT_EQ(create_response->status.code(), scada::StatusCode::Good);

  const auto activated = WaitAwaitable(
      executor,
      runtime.Handle(connection,
                     OpcUaRequestBody{OpcUaActivateSessionRequest{
                         .session_id = create_response->session_id,
                         .authentication_token = create_response->authentication_token,
                         .user_name = scada::LocalizedText{u"operator"},
                         .password = scada::LocalizedText{u"secret"},
                     }}));
  const auto* activate_response =
      std::get_if<OpcUaActivateSessionResponse>(&activated);
  ASSERT_NE(activate_response, nullptr);
  ASSERT_EQ(activate_response->status.code(), scada::StatusCode::Good);

  ReadRequest request{
      .inputs = {{.node_id = NumericNode(1),
                  .attribute_id = scada::AttributeId::DisplayName}}};
  EXPECT_CALL(attribute_service, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(context.user_id(), expected_user_id);
        EXPECT_THAT(*inputs, ElementsAre(request.inputs[0]));
        callback(scada::StatusCode::Good,
                 {scada::DataValue{scada::LocalizedText{u"Pump"},
                                   {},
                                   base::Time{},
                                   base::Time{}}});
      }));

  const auto response = WaitAwaitable(
      executor, runtime.Handle(connection, OpcUaRequestBody{request}));
  const auto* read_response = std::get_if<ReadResponse>(&response);
  ASSERT_NE(read_response, nullptr);
  EXPECT_EQ(read_response->status.code(), scada::StatusCode::Good);
  ASSERT_EQ(read_response->results.size(), 1u);
  EXPECT_EQ(read_response->results[0].value,
            scada::Variant{scada::LocalizedText{u"Pump"}});
}

}  // namespace
}  // namespace opcua
