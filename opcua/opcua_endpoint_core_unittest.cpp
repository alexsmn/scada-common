#include "opcua/opcua_endpoint_core.h"

#include "scada/attribute_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace scada::opcua_endpoint {
namespace {

class OpcUaEndpointCoreTest : public Test {
 protected:
  static scada::NodeId NumericNode(scada::NumericId id,
                                   scada::NamespaceIndex ns = 2) {
    return {id, ns};
  }

  StrictMock<scada::MockAttributeService> attribute_service_;
};

TEST_F(OpcUaEndpointCoreTest, MakeServiceContext_SetsRequestedUserId) {
  const auto user_id = NumericNode(42, 3);
  const auto context = MakeServiceContext(user_id);
  EXPECT_EQ(context.user_id(), user_id);
}

TEST_F(OpcUaEndpointCoreTest, Read_NormalizesBadWrongNodeIdInCallbackPath) {
  const auto user_id = NumericNode(99, 4);
  auto inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      std::vector<scada::ReadValueId>{
          {.node_id = NumericNode(1), .attribute_id = scada::AttributeId::Value}});

  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& actual_context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& actual_inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(actual_context.user_id(), user_id);
        EXPECT_EQ(actual_inputs, inputs);
        callback(scada::StatusCode::Good,
                 {scada::MakeReadError(scada::StatusCode::Bad_WrongNodeId)});
      }));

  std::optional<scada::Status> status;
  std::optional<std::vector<scada::DataValue>> results;
  Read(attribute_service_, MakeServiceContext(user_id), inputs,
       [&](scada::Status actual_status,
           std::vector<scada::DataValue> actual_results) {
         status = std::move(actual_status);
         results = std::move(actual_results);
       });

  ASSERT_TRUE(status.has_value());
  ASSERT_TRUE(results.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Good);
  ASSERT_EQ(results->size(), 1u);
  EXPECT_EQ(scada::Status((*results)[0].status_code).full_code(), 0x80340000u);
}

TEST_F(OpcUaEndpointCoreTest, Write_ForwardsUserContextAndResults) {
  const auto user_id = NumericNode(101, 4);
  auto inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      std::vector<scada::WriteValue>{{
          .node_id = NumericNode(3),
          .attribute_id = scada::AttributeId::Value,
          .value = scada::Variant{scada::Int32{7}},
      }});

  EXPECT_CALL(attribute_service_, Write(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& actual_context,
                           const std::shared_ptr<const std::vector<scada::WriteValue>>& actual_inputs,
                           const scada::WriteCallback& callback) {
        EXPECT_EQ(actual_context.user_id(), user_id);
        EXPECT_EQ(actual_inputs, inputs);
        callback(scada::StatusCode::Good, {scada::StatusCode::Good});
      }));

  std::optional<scada::Status> status;
  std::optional<std::vector<scada::StatusCode>> results;
  Write(attribute_service_, MakeServiceContext(user_id), inputs,
        [&](scada::Status actual_status,
            std::vector<scada::StatusCode> actual_results) {
          status = std::move(actual_status);
          results = std::move(actual_results);
        });

  ASSERT_TRUE(status.has_value());
  ASSERT_TRUE(results.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Good);
  EXPECT_THAT(*results, ElementsAre(scada::StatusCode::Good));
}

}  // namespace
}  // namespace scada::opcua_endpoint
