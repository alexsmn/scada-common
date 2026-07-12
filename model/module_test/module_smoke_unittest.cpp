// Multi-import interop smoke test for the scada.model module facade:
// imports scada.base, scada.core, and scada.model together (not relying on
// the export-import collapse), combined with textual includes of the
// internal-linkage node-id/namespace constant headers - the documented
// consumption pattern for model constants.

#include "model/namespaces.h"
#include "model/scada_node_ids.h"

#include <string>
#include <string_view>

#include <gtest/gtest.h>

// Imports after the textual includes (AppleClang 21 libc++ merging bug).
import scada.base;
import scada.core;
import scada.model;

namespace scada_model_module {
namespace {

TEST(ScadaModelModuleSmoke, NestedNodeIdsWithConstantHeaders) {
  // scada::id::* and NamespaceIndexes::* come from the textual includes
  // (internal linkage - never exported); the functions come from the import.
  scada::NodeId parent = scada::id::PropertyCategories;
  scada::NodeId nested = MakeNestedNodeId(parent, "sub");
  EXPECT_TRUE(IsNestedNodeId(nested));
  EXPECT_EQ(nested.namespace_index(), NamespaceIndexes::SCADA);

  scada::NodeId root;
  std::string_view nested_name;
  EXPECT_TRUE(GetRootNestedNodeId(nested, root, nested_name));
  EXPECT_EQ(root, parent);
  EXPECT_EQ(nested_name, "sub");
}

TEST(ScadaModelModuleSmoke, ScadaStringsAndNamespaces) {
  scada::NodeId node_id = NodeIdFromScadaString("Server!CPU");
  EXPECT_FALSE(node_id.is_null());
  EXPECT_FALSE(NodeIdToScadaString(node_id).empty());

  EXPECT_EQ(GetNamespaceName(NamespaceIndexes::NS0), "NS0");
  EXPECT_EQ(FindNamespaceIndexByName("NS0"), 0);

  // cfg enums/constants from static_types.h (external linkage - exported).
  EXPECT_EQ(cfg::NUM_CHANNELS, 2);
  cfg::ModbusMode mode = cfg::ModbusMode::MASTER;
  EXPECT_NE(mode, cfg::ModbusMode::SLAVE);

  // Names from all three imports coexist in one TU.
  base::Check(true, "model module smoke");
  EXPECT_EQ(Format(1), "1");
  EXPECT_TRUE(scada::Status{scada::StatusCode::Good});
}

}  // namespace
}  // namespace scada_model_module
