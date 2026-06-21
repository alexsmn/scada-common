#include "attribute_service_impl.h"

#include "address_space/test/test_address_space.h"
#include "base/test/awaitable_test.h"
#include "scada/data_value.h"
#include "scada/service_context.h"
#include "scada/test/status_matchers.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

TEST(AttributeServiceImpl, CoroutineReadReturnsSyncResults) {
  TestAddressSpace address_space;
  TestExecutor executor;

  auto inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      std::vector<scada::ReadValueId>{
          {.node_id = address_space.kTestNode1Id,
           .attribute_id = scada::AttributeId::DisplayName},
          {.node_id = address_space.MakeNestedNodeId(
               address_space.kTestNode1Id, address_space.kTestProp1Id),
           .attribute_id = scada::AttributeId::Value}});

  ASSERT_OK_AND_ASSIGN(
      auto results,
      WaitAwaitable(executor, address_space.attribute_service_impl.Read(
                                  scada::ServiceContext{}, inputs)));

  ASSERT_EQ(results.size(), 2u);
  EXPECT_EQ(results[0].status_code, scada::StatusCode::Good);
  EXPECT_EQ(results[0].value,
            scada::Variant{scada::LocalizedText{u"TestNode1DisplayName"}});
  EXPECT_EQ(results[1].status_code, scada::StatusCode::Good);
  EXPECT_EQ(results[1].value, scada::Variant{"TestNode1.TestProp1.Value"});
}

TEST(AttributeServiceImpl, CoroutineWriteReturnsSyncResults) {
  TestAddressSpace address_space;
  TestExecutor executor;

  auto inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      std::vector<scada::WriteValue>{
          {.node_id = address_space.kTestNode1Id,
           .attribute_id = scada::AttributeId::Value,
           .value = scada::Variant{scada::Int32{42}}}});

  ASSERT_OK_AND_ASSIGN(
      auto results,
      WaitAwaitable(executor, address_space.attribute_service_impl.Write(
                                  scada::ServiceContext{}, inputs)));

  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0], scada::StatusCode::Bad_WrongNodeId);
}

TEST(AttributeServiceImpl, ReadsMandatoryVariableAttributes) {
  TestAddressSpace address_space;

  const std::vector<scada::ReadValueId> inputs{
      {.node_id = address_space.kTestProp1Id,
       .attribute_id = scada::AttributeId::ValueRank},
      {.node_id = address_space.kTestProp1Id,
       .attribute_id = scada::AttributeId::AccessLevel},
      {.node_id = address_space.kTestProp1Id,
       .attribute_id = scada::AttributeId::UserAccessLevel},
      {.node_id = address_space.kTestProp1Id,
       .attribute_id = scada::AttributeId::Historizing},
      {.node_id = address_space.kTestProp1Id,
       .attribute_id = scada::AttributeId::MinimumSamplingInterval}};

  const auto results = address_space.sync_attribute_service_impl.Read(
      scada::ServiceContext{}, inputs);

  ASSERT_EQ(results.size(), 5u);
  for (const auto& result : results)
    EXPECT_EQ(result.status_code, scada::StatusCode::Good);
  EXPECT_EQ(results[0].value, scada::Variant{scada::Int32{-1}});
  EXPECT_EQ(results[1].value, scada::Variant{scada::UInt8{0x01}});
  EXPECT_EQ(results[2].value, scada::Variant{scada::UInt8{0x01}});
  EXPECT_EQ(results[3].value, scada::Variant{false});
  EXPECT_EQ(results[4].value, scada::Variant{0.0});
}

TEST(AttributeServiceImpl, ReadNestedNodeWithSinglePartName) {
  TestAddressSpace address_space;

  std::vector<scada::ReadValueId> inputs{
      {.node_id = address_space.MakeNestedNodeId(address_space.kTestNode1Id,
                                                 address_space.kTestProp1Id),
       .attribute_id = scada::AttributeId::Value}};

  auto results = address_space.sync_attribute_service_impl.Read(
      scada::ServiceContext{}, inputs);

  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].status_code, scada::StatusCode::Good);
  EXPECT_EQ(results[0].value, scada::Variant{"TestNode1.TestProp1.Value"});
}

}  // namespace
