#include "address_space/address_space_xml.h"

#include "address_space/address_space_impl2.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "address_space/scada_address_space.h"
#include "common/test/node_state_matcher.h"
#include "model/node_id_util.h"

#include <gmock/gmock.h>

#include <cstdlib>

namespace {

void BuildScadaAddressSpace(AddressSpaceImpl2& address_space) {
  GenericNodeFactory node_factory{address_space};
  ScadaAddressSpaceBuilder{address_space, node_factory}.BuildAll();
}

std::vector<scada::NodeState> SortedNodeStates(
    const scada::AddressSpace& address_space) {
  auto node_states = scada::MakeNodeStates(address_space);
  std::ranges::sort(node_states, {}, [](const scada::NodeState& node_state) {
    return NodeIdToScadaString(node_state.node_id);
  });
  return node_states;
}

}  // namespace

TEST(AddressSpaceXml, RoundTripsScadaAddressSpace) {
  AddressSpaceImpl2 source_address_space;
  BuildScadaAddressSpace(source_address_space);

  const auto* export_path = std::getenv("SCADA_WRITE_STATIC_ADDRESS_SPACE_XML");
  const auto path =
      export_path && *export_path
          ? std::filesystem::path{export_path}
          : std::filesystem::temp_directory_path() / "scada_static.xml";

  ASSERT_TRUE(scada::SaveAddressSpaceXml(path, source_address_space));

  AddressSpaceImpl2 loaded_address_space;
  GenericNodeFactory node_factory{loaded_address_space};
  ASSERT_TRUE(
      scada::LoadAddressSpaceXml(path, loaded_address_space, node_factory));

  EXPECT_THAT(SortedNodeStates(loaded_address_space),
              testing::UnorderedPointwise(
                  NodeStateEq(), SortedNodeStates(source_address_space)));
}
