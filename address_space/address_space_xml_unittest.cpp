#include "address_space/address_space_xml.h"

#include "address_space/address_space_impl2.h"
#include "address_space/address_space_impl3.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "common/test/node_state_matcher.h"
#include "model/node_id_util.h"
#include "model/static_nodesets.h"

#include <gmock/gmock.h>

#include <filesystem>

namespace {

std::vector<scada::NodeState> SortedNodeStates(
    const scada::AddressSpace& address_space) {
  auto node_states = scada::MakeNodeStates(address_space);
  std::ranges::sort(node_states, {}, [](const scada::NodeState& node_state) {
    return NodeIdToScadaString(node_state.node_id);
  });
  return node_states;
}

}  // namespace

// Loads the partitioned static nodesets, serializes them back out, reloads the
// serialized form, and verifies the round trip preserves every node. This keeps
// the XML codec covered now that the static address space is sourced from
// committed files rather than a programmatic builder.
TEST(AddressSpaceXml, RoundTripsStaticNodesets) {
  AddressSpaceImpl3 source_address_space;

  const auto path =
      std::filesystem::temp_directory_path() / "scada_static_roundtrip.xml";
  ASSERT_TRUE(scada::SaveAddressSpaceXml(path, source_address_space));

  AddressSpaceImpl2 loaded_address_space;
  GenericNodeFactory node_factory{loaded_address_space};
  ASSERT_TRUE(
      scada::LoadAddressSpaceXml(path, loaded_address_space, node_factory));

  EXPECT_THAT(SortedNodeStates(loaded_address_space),
              testing::UnorderedPointwise(
                  NodeStateEq(), SortedNodeStates(source_address_space)));
}

// Loading the multi-file static set must succeed: LoadStaticAddressSpace fails
// with a Bad status if any supertype/parent/reference target is missing, so a
// successful load is a referential-integrity check across all partition files.
TEST(AddressSpaceXml, StaticNodesetsResolveAcrossFiles) {
  AddressSpaceImpl2 address_space;
  GenericNodeFactory node_factory{address_space};
  EXPECT_TRUE(scada::LoadStaticAddressSpace(
      scada::GetScadaStaticNodesetSourcePaths(), address_space, node_factory));
}
