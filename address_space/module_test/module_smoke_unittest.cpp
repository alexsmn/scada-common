// Smoke test for the scada.address_space module facade: names come from
// `import scada.address_space;` only, including scada.core names
// re-exported through `export import`.

#include <gtest/gtest.h>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.address_space;

namespace scada_address_space_module {
namespace {

TEST(ScadaAddressSpaceModuleSmoke, StandardAddressSpace) {
  // AddressSpaceImpl2 owns the standard node substrate. scada::id constants
  // are internal-linkage and never exported; the literal 84 is ns=0;i=84
  // (RootFolder).
  AddressSpaceImpl2 address_space;
  EXPECT_NE(address_space.GetNode(scada::NodeId{84, 0}), nullptr);
}

TEST(ScadaAddressSpaceModuleSmoke, LocalMethodServiceLinks) {
  // Out-of-line coroutine from the pilot TU links across the boundary.
  scada::LocalMethodService service;
  (void)service;
  EXPECT_TRUE(scada::Status{scada::StatusCode::Good});
}

}  // namespace
}  // namespace scada_address_space_module
