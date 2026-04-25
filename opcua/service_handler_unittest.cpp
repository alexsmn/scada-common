#include "opcua/service_handler.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "scada/attribute_service_mock.h"
#include "scada/coroutine_services.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/node_management_service_mock.h"
#include "scada/service_context.h"
#include "scada/view_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace opcua {
namespace {

scada::NodeId NumericNode(scada::NumericId id, scada::NamespaceIndex ns = 2) {
  return {id, ns};
}

TEST(ServiceHandlerCanonicalTest,
     HandleRead_UsesCanonicalCommonSurfaceAndUserContext) {
  StrictMock<scada::MockAttributeService> attribute_service;
  StrictMock<scada::MockViewService> view_service;
  StrictMock<scada::MockHistoryService> history_service;
  StrictMock<scada::MockMethodService> method_service;
  StrictMock<scada::MockNodeManagementService> node_management_service;
  const auto executor = std::make_shared<TestExecutor>();
  const auto any_executor = MakeTestAnyExecutor(executor);
  scada::CallbackToCoroutineAttributeServiceAdapter attribute_service_adapter{
      any_executor, attribute_service};
  scada::CallbackToCoroutineViewServiceAdapter view_service_adapter{
      any_executor, view_service};
  scada::CallbackToCoroutineHistoryServiceAdapter history_service_adapter{
      any_executor, history_service};
  scada::CallbackToCoroutineMethodServiceAdapter method_service_adapter{
      any_executor, method_service};
  scada::CallbackToCoroutineNodeManagementServiceAdapter
      node_management_service_adapter{any_executor, node_management_service};
  const auto user_id = NumericNode(700, 5);
  ServiceHandler handler{{attribute_service_adapter,
                          view_service_adapter,
                          history_service_adapter,
                          method_service_adapter,
                          node_management_service_adapter,
                          user_id}};

  ReadRequest request{
      .inputs = {{.node_id = NumericNode(1),
                  .attribute_id = scada::AttributeId::DisplayName}}};
  EXPECT_CALL(attribute_service, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(context.user_id(), user_id);
        EXPECT_THAT(*inputs, ElementsAre(request.inputs[0]));
        callback(scada::StatusCode::Good,
                 {scada::DataValue{scada::LocalizedText{u"Pump"},
                                   {},
                                   base::Time{},
                                   base::Time{}}});
      }));

  const auto response = WaitAwaitable(executor, handler.Handle(request));
  const auto* read_response = std::get_if<ReadResponse>(&response);
  ASSERT_NE(read_response, nullptr);
  EXPECT_EQ(read_response->status.code(), scada::StatusCode::Good);
  ASSERT_EQ(read_response->results.size(), 1u);
  EXPECT_EQ(read_response->results[0].value,
            scada::Variant{scada::LocalizedText{u"Pump"}});
}

}  // namespace
}  // namespace opcua
