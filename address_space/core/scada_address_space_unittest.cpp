#include <gmock/gmock.h>

#include "base/logger.h"
#include "core/configuration_impl.h"
#include "core/node_utils.h"
#include "core/standard_address_space.h"
#include "core/standard_node_ids.h"
#include "core/type_definition.h"

TEST(Types, Test) {
  ConfigurationImpl cfg(std::make_shared<NullLogger>());

  auto* has_component_ref =
      scada::AsReferenceType(cfg.GetNode(scada::id::HasComponent));
  ASSERT_NE(has_component_ref, nullptr);
  EXPECT_EQ(has_component_ref->GetNodeClass(), scada::NodeClass::ReferenceType);
  EXPECT_TRUE(scada::IsSubtypeOf(*has_component_ref,
                                 scada::id::HierarchicalReferences));
}
