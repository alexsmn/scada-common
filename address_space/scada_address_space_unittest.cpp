#include "address_space/scada_address_space.h"

#include "address_space/address_space_impl2.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "core/standard_node_ids.h"

#include <gmock/gmock.h>

TEST(ScadaAddressSpace, Test) {
  AddressSpaceImpl2 address_space;

  GenericNodeFactory node_factory{address_space};
  ScadaAddressSpaceBuilder{address_space, node_factory}.BuildAll();

  auto* has_component_ref =
      scada::AsReferenceType(address_space.GetNode(scada::id::HasComponent));
  ASSERT_TRUE(has_component_ref);
  EXPECT_EQ(has_component_ref->GetNodeClass(), scada::NodeClass::ReferenceType);
  EXPECT_TRUE(scada::IsSubtypeOf(*has_component_ref,
                                 scada::id::HierarchicalReferences));
}
