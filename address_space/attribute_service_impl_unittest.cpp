#include "attribute_service_impl.h"

#include "address_space/test/test_address_space.h"
#include "base/test/awaitable_test.h"
#include "scada/data_value.h"
#include "scada/privileges.h"
#include "scada/service_context.h"
#include "scada/test/status_matchers.h"

#include <cstdint>

#include <gmock/gmock.h>

using namespace testing;

namespace {

TEST(AttributeServiceImpl, CoroutineReadReturnsSyncResults) {
  TestAddressSpace address_space;
  TestExecutor executor;

  std::vector<scada::ReadValueId> inputs{
      {.node_id = address_space.kTestNode1Id,
       .attribute_id = scada::AttributeId::DisplayName},
      {.node_id = address_space.MakeNestedNodeId(address_space.kTestNode1Id,
                                                 address_space.kTestProp1Id),
       .attribute_id = scada::AttributeId::Value}};

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

  std::vector<scada::WriteValue> inputs{
      {.node_id = address_space.kTestNode1Id,
       .attribute_id = scada::AttributeId::Value,
       .value = scada::Variant{scada::Int32{42}}}};

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

TEST(AttributeServiceImpl, UserAccessLevelIsBoundedByNodeAccessLevel) {
  TestAddressSpace address_space;

  // The static node advertises only CurrentRead, so even a fully privileged
  // caller sees UserAccessLevel == CurrentRead: a user cannot gain write access
  // the node itself does not offer (OPC UA Part 3 §5.6.2).
  const std::uint32_t root_rights =
      (std::uint32_t{1} << static_cast<int>(scada::Privilege::Configure)) |
      (std::uint32_t{1} << static_cast<int>(scada::Privilege::Control));
  const scada::ServiceContext context =
      scada::ServiceContext{}
          .with_user_id(scada::NodeId{1, 1})
          .with_user_rights(root_rights);

  const std::vector<scada::ReadValueId> inputs{
      {.node_id = address_space.kTestProp1Id,
       .attribute_id = scada::AttributeId::AccessLevel},
      {.node_id = address_space.kTestProp1Id,
       .attribute_id = scada::AttributeId::UserAccessLevel}};

  const auto results =
      address_space.sync_attribute_service_impl.Read(context, inputs);

  ASSERT_EQ(results.size(), 2u);
  EXPECT_EQ(results[0].value, scada::Variant{scada::UInt8{0x01}});
  EXPECT_EQ(results[1].value, scada::Variant{scada::UInt8{0x01}});
}

TEST(AttributeServiceImpl, MethodUserExecutableFollowsCallPermission) {
  TestAddressSpace address_space;

  const scada::NodeId method_id{700, TestAddressSpace::kNamespaceIndex};
  address_space.CreateNode({.node_id = method_id,
                            .node_class = scada::NodeClass::Method,
                            .parent_id = address_space.kTestNode1Id,
                            .reference_type_id = scada::id::HasComponent,
                            .attributes = scada::NodeAttributes{}
                                              .set_browse_name("TestMethod")
                                              .set_display_name(u"TestMethod")});

  const std::uint32_t control_rights =
      std::uint32_t{1} << static_cast<int>(scada::Privilege::Control);

  const auto read = [&](scada::AttributeId attribute_id,
                        const scada::ServiceContext& context) {
    const std::vector<scada::ReadValueId> inputs{
        {.node_id = method_id, .attribute_id = attribute_id}};
    return address_space.sync_attribute_service_impl.Read(context, inputs).at(0);
  };

  // Executable is always true for a method.
  EXPECT_EQ(read(scada::AttributeId::Executable, scada::ServiceContext{}).value,
            scada::Variant{true});

  // UserExecutable tracks the Call permission: an Operator (Control) may call;
  // a plain authenticated Observer and an anonymous caller may not.
  const scada::ServiceContext operator_context =
      scada::ServiceContext{}.with_user_id(scada::NodeId{1, 1}).with_user_rights(
          control_rights);
  const scada::ServiceContext observer_context =
      scada::ServiceContext{}.with_user_id(scada::NodeId{1, 1});

  EXPECT_EQ(read(scada::AttributeId::UserExecutable, operator_context).value,
            scada::Variant{true});
  EXPECT_EQ(read(scada::AttributeId::UserExecutable, observer_context).value,
            scada::Variant{false});
  EXPECT_EQ(read(scada::AttributeId::UserExecutable, scada::ServiceContext{}).value,
            scada::Variant{false});
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
