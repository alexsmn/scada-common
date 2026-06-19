#include "address_space/address_space_impl3.h"

#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

// Verifies the static address space loaded from the committed nodeset files
// (via AddressSpaceImpl3) preserves the standard OPC UA reference-type
// hierarchy.
TEST(ScadaAddressSpace, Test) {
  AddressSpaceImpl3 address_space;

  auto* has_component_ref =
      scada::AsReferenceType(address_space.GetNode(scada::id::HasComponent));
  ASSERT_TRUE(has_component_ref);
  EXPECT_EQ(has_component_ref->GetNodeClass(), scada::NodeClass::ReferenceType);
  EXPECT_TRUE(scada::IsSubtypeOf(*has_component_ref,
                                 scada::id::HierarchicalReferences));
}
