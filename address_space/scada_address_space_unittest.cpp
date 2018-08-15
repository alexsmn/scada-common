#include <gmock/gmock.h>

#include "address_space/address_space_impl.h"
#include "address_space/node_utils.h"
#include "address_space/standard_address_space.h"
#include "address_space/type_definition.h"
#include "base/logger.h"
#include "core/standard_node_ids.h"

TEST(Types, Test) {
  AddressSpaceImpl2 cfg(std::make_shared<NullLogger>());

  auto* has_component_ref =
      scada::AsReferenceType(cfg.GetNode(scada::id::HasComponent));
  ASSERT_TRUE(has_component_ref);
  EXPECT_EQ(has_component_ref->GetNodeClass(), scada::NodeClass::ReferenceType);
  EXPECT_TRUE(scada::IsSubtypeOf(*has_component_ref,
                                 scada::id::HierarchicalReferences));
}
