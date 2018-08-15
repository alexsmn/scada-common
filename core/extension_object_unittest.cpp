#include "core/extension_object.h"

#include <gmock/gmock.h>

namespace scada {

struct TestObject {};

TEST(ExtensionObject, ConstructExtract) {
  const ExpandedNodeId kTestTypeId{123, "uri", 22};
  TestObject test_object;
  ExtensionObject extension_object{kTestTypeId, std::move(test_object)};
  EXPECT_EQ(kTestTypeId, extension_object.data_type_id());
  auto* saved_object = std::any_cast<TestObject>(&extension_object.value());
  ASSERT_TRUE(saved_object);
}

}  // namespace scada
