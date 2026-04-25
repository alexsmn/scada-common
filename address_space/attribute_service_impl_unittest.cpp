#include "attribute_service_impl.h"

#include "address_space/test/test_address_space.h"
#include "base/test/awaitable_test.h"
#include "scada/data_value.h"
#include "scada/service_context.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

TEST(AttributeServiceImpl, CoroutineReadReturnsSyncResults) {
  TestAddressSpace address_space;
  const auto executor = std::make_shared<TestExecutor>();

  auto inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      std::vector<scada::ReadValueId>{
          {.node_id = address_space.kTestNode1Id,
           .attribute_id = scada::AttributeId::DisplayName},
          {.node_id = address_space.MakeNestedNodeId(address_space.kTestNode1Id,
                                                     address_space.kTestProp1Id),
           .attribute_id = scada::AttributeId::Value}});

  auto [status, results] = WaitAwaitable(
      executor, address_space.attribute_service_impl.Read(scada::ServiceContext{},
                                                          inputs));

  EXPECT_TRUE(status);
  ASSERT_EQ(results.size(), 2u);
  EXPECT_EQ(results[0].status_code, scada::StatusCode::Good);
  EXPECT_EQ(results[0].value,
            scada::Variant{scada::LocalizedText{u"TestNode1DisplayName"}});
  EXPECT_EQ(results[1].status_code, scada::StatusCode::Good);
  EXPECT_EQ(results[1].value, scada::Variant{"TestNode1.TestProp1.Value"});
}

TEST(AttributeServiceImpl, CoroutineWriteReturnsSyncResults) {
  TestAddressSpace address_space;
  const auto executor = std::make_shared<TestExecutor>();

  auto inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      std::vector<scada::WriteValue>{
          {.node_id = address_space.kTestNode1Id,
           .attribute_id = scada::AttributeId::Value,
           .value = scada::Variant{scada::Int32{42}}}});

  auto [status, results] = WaitAwaitable(
      executor, address_space.attribute_service_impl.Write(
                    scada::ServiceContext{}, inputs));

  EXPECT_TRUE(status);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0], scada::StatusCode::Bad_WrongNodeId);
}

}  // namespace
