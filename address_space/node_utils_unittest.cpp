#include "address_space/node_utils.h"

#include "address_space/address_space_impl2.h"
#include "address_space/address_space_xml.h"
#include "address_space/generic_node_factory.h"
#include "address_space/object.h"
#include "address_space/test/test_address_space.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "model/static_nodesets.h"

#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

namespace scada {
namespace {

MATCHER_P2(IsCreatable, reference_type_id, type_definition_id, "") {
  return arg.reference_type_id == reference_type_id &&
         arg.type_definition_id == type_definition_id;
}

}  // namespace

TEST(NodeUtils, GetForwardReferenceNodes) {
  TestAddressSpace address_space;

  auto* parent = address_space.GetNode(address_space.kTestNode2Id);
  auto* property1 = address_space.GetNode(MakeNestedNodeId(
      address_space.kTestNode2Id, address_space.kTestProp1BrowseName));
  auto* property2 = address_space.GetNode(MakeNestedNodeId(
      address_space.kTestNode2Id, address_space.kTestProp2BrowseName));

  ASSERT_TRUE(parent);
  ASSERT_TRUE(property1);
  ASSERT_TRUE(property2);
  ASSERT_TRUE(
      scada::FindReference(*parent, address_space.kTestReferenceTypeId, true));

  EXPECT_THAT(GetForwardReferenceNodes(*parent, scada::id::Aggregates),
              UnorderedElementsAre(property1, property2));
}

// GetCreatableChildTypes reads the OptionalPlaceholder InstanceDeclarations that
// replaced the proprietary `Creates` reference. Loads the real static SCADA
// address space so the assertions exercise the shipped nodeset placeholders.
TEST(NodeUtils, GetCreatableChildTypesFromPlaceholders) {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};
  ASSERT_TRUE(
      LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(), space, factory));

  // The DataItems folder is an instance of DataItemsFolderType, whose
  // <DataGroup> placeholder marks DataGroupType as creatable via Organizes.
  auto* data_items = space.GetNode(data_items::id::DataItems);
  ASSERT_TRUE(data_items);
  EXPECT_THAT(
      GetCreatableChildTypes(*data_items),
      UnorderedElementsAre(
          IsCreatable(id::Organizes, data_items::id::DataGroupType)));

  // DataGroupType itself declares nested DataGroups and DataItems.
  auto* data_group_type = space.GetNode(data_items::id::DataGroupType);
  ASSERT_TRUE(data_group_type);
  EXPECT_THAT(
      GetCreatableChildTypes(*data_group_type),
      UnorderedElementsAre(
          IsCreatable(id::Organizes, data_items::id::DataGroupType),
          IsCreatable(id::Organizes, data_items::id::DataItemType)));

  // A type with no placeholder declarations returns nothing.
  auto* alias = space.GetNode(NodeId{id::ModellingRule_Mandatory});
  if (alias)
    EXPECT_TRUE(GetCreatableChildTypes(*alias).empty());
}

}  // namespace scada
