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

MATCHER_P3(IsReference, reference_type_id, forward, node_id, "") {
  return arg.reference_type_id == reference_type_id && arg.forward == forward &&
         arg.node_id == node_id;
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

// GetCreatableChildTypes reads the OptionalPlaceholder InstanceDeclarations
// that replaced the proprietary `Creates` reference. Loads the real static
// SCADA address space so the assertions exercise the shipped nodeset
// placeholders.
TEST(NodeUtils, GetCreatableChildTypesFromPlaceholders) {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};
  ASSERT_TRUE(LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(), space,
                                     factory));

  // The DataItems folder is an instance of DataItemsFolderType, whose
  // <DataGroup> placeholder marks DataGroupType as creatable via Organizes.
  auto* data_items = space.GetNode(data_items::id::DataItems);
  ASSERT_TRUE(data_items);
  EXPECT_THAT(GetCreatableChildTypes(*data_items),
              UnorderedElementsAre(
                  IsCreatable(id::Organizes, data_items::id::DataGroupType)));

  // DataGroupType itself declares nested DataGroups and DataItems.
  auto* data_group_type = space.GetNode(data_items::id::DataGroupType);
  ASSERT_TRUE(data_group_type);
  EXPECT_THAT(GetCreatableChildTypes(*data_group_type),
              UnorderedElementsAre(
                  IsCreatable(id::Organizes, data_items::id::DataGroupType),
                  IsCreatable(id::Organizes, data_items::id::DataItemType)));

  // A type with no placeholder declarations returns nothing.
  auto* alias = space.GetNode(NodeId{id::ModellingRule_Mandatory});
  if (alias)
    EXPECT_TRUE(GetCreatableChildTypes(*alias).empty());
}

// MakeNodeState must keep hierarchical references beyond the target's single
// parent slot: an extra Organizes edge between two instances (e.g. a device
// organized under another device as well as under its folder) exists only in
// the graph, so the snapshot has to export it from the source node or
// consumers rebuilt from NodeStates (StaticNodeService, XML round-trips) lose
// the edge. The target's primary parent edge stays out of `references` — it is
// carried by the target's parent_id/reference_type_id.
TEST(NodeUtils, MakeNodeStateKeepsExtraHierarchicalReferences) {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};

  const NodeId node1_id{1000, 1};
  const NodeId node2_id{1001, 1};

  auto [status1, node1] = factory.CreateNode(
      NodeState{node1_id, NodeClass::Object, NodeId{id::BaseObjectType},
                NodeId{id::ObjectsFolder}, NodeId{id::Organizes},
                NodeAttributes{}.set_display_name(u"Node 1")});
  ASSERT_TRUE(status1);
  ASSERT_TRUE(node1);

  auto [status2, node2] = factory.CreateNode(
      NodeState{node2_id, NodeClass::Object, NodeId{id::BaseObjectType},
                NodeId{id::ObjectsFolder}, NodeId{id::Organizes},
                NodeAttributes{}.set_display_name(u"Node 2")});
  ASSERT_TRUE(status2);
  ASSERT_TRUE(node2);

  AddReference(space, id::Organizes, node1_id, node2_id);

  // The extra edge is exported from its source node.
  EXPECT_THAT(MakeNodeState(*node1).references,
              Contains(IsReference(NodeId{id::Organizes}, true, node2_id)));

  // The target still reports the folder as its primary parent.
  const NodeState node2_state = MakeNodeState(*node2);
  EXPECT_EQ(node2_state.parent_id, NodeId{id::ObjectsFolder});
  EXPECT_EQ(node2_state.reference_type_id, NodeId{id::Organizes});

  // Primary parent edges are not duplicated into the parent's snapshot.
  auto* objects_folder = space.GetNode(NodeId{id::ObjectsFolder});
  ASSERT_TRUE(objects_folder);
  EXPECT_THAT(
      MakeNodeState(*objects_folder).references,
      Not(Contains(IsReference(NodeId{id::Organizes}, true, node2_id))));
}

}  // namespace scada
