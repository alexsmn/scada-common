// Regression guard for the static SCADA address space across the UANodeSet2
// migration (see common/model/docs/uanodeset-migration.md). Loads the static
// nodesets (via the format-detecting LoadStaticAddressSpace) and compares the
// resulting address space to a committed golden dump captured from the original
// scada-node-state-v1 model, so the cut-over to UANodeSet2 -- and any later
// nodeset edit -- is proven to preserve the exact node/reference structure.

#include "address_space/address_space_xml.h"

#include "address_space/address_space_impl2.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "common/node_state.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/static_nodesets.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace scada {
namespace {

std::string Canonicalize(const NodeState& n) {
  std::vector<std::string> refs;
  for (const auto& r : n.references) {
    refs.push_back(std::format("{}{}{}", NodeIdToScadaString(r.reference_type_id),
                               r.forward ? "->" : "<-",
                               NodeIdToScadaString(r.node_id)));
  }
  std::ranges::sort(refs);
  std::string joined;
  for (const auto& r : refs)
    joined += r + ";";
  return std::format(
      "class={} browse={}:{} typedef={} parent={} parentRef={} supertype={} "
      "dataType={} value={} refs=[{}]",
      static_cast<int>(n.node_class), n.attributes.browse_name.namespace_index(),
      std::string{n.attributes.browse_name.name()},
      NodeIdToScadaString(n.type_definition_id),
      NodeIdToScadaString(n.parent_id),
      NodeIdToScadaString(n.reference_type_id),
      NodeIdToScadaString(n.supertype_id),
      NodeIdToScadaString(n.attributes.data_type),
      n.attributes.value ? "set" : "none", joined);
}

std::map<std::string, std::string> LoadAndDump() {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};
  const Status status =
      LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(), space, factory);
  EXPECT_TRUE(status);
  std::map<std::string, std::string> out;
  for (const auto& node_state : MakeNodeStates(space))
    out.emplace(NodeIdToScadaString(node_state.node_id), Canonicalize(node_state));
  return out;
}

std::map<std::string, std::string> ReadGolden() {
  const std::filesystem::path golden =
      std::filesystem::path{__FILE__}.parent_path() /
      "scada_address_space_golden.txt";
  std::map<std::string, std::string> out;
  std::ifstream in{golden};
  std::string line;
  while (std::getline(in, line)) {
    const auto tab = line.find('\t');
    if (tab != std::string::npos)
      out.emplace(line.substr(0, tab), line.substr(tab + 1));
  }
  return out;
}

// The static SCADA address space (now loaded from opcua_base.xml +
// Scada.NodeSet2.xml) must match the golden captured from the original
// scada-node-state-v1 model, node-for-node.
TEST(ScadaAddressSpace, MatchesGolden) {
  const auto golden = ReadGolden();
  ASSERT_FALSE(golden.empty()) << "golden fixture missing";
  const auto actual = LoadAndDump();

  for (const auto& [id, canonical] : golden)
    if (!actual.contains(id))
      ADD_FAILURE() << "node missing from loaded address space: " << id;
  for (const auto& [id, canonical] : actual)
    if (!golden.contains(id))
      ADD_FAILURE() << "unexpected node in loaded address space: " << id;
  for (const auto& [id, canonical] : golden) {
    auto it = actual.find(id);
    if (it != actual.end())
      EXPECT_EQ(canonical, it->second) << "node " << id << " differs from golden";
  }
}

// InverseName is parsed from the UANodeSet <InverseName> element and
// materialized on the ReferenceType node (OPC UA Part 3 §5.3.2,
// https://reference.opcfoundation.org/Core/Part3/v105/docs/5.3.2).
TEST(ScadaAddressSpace, ReferenceTypeInverseName) {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};
  ASSERT_TRUE(LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(), space,
                                     factory));

  const auto* reference_type =
      AsReferenceType(space.GetNode(devices::id::HasTransmissionItem));
  ASSERT_TRUE(reference_type);
  EXPECT_EQ(ToString(reference_type->inverse_name()), "TransmissionItemOf");
}

// An instance node parented through a model-defined hierarchical
// ReferenceType (HasTransmissionItem, a HasComponent subtype) is materialized
// with that parent — the OptionalPlaceholder InstanceDeclarations on the
// protocol device types rely on this (OPC UA Part 3 §6.4.4.4.4).
TEST(ScadaAddressSpace, PlaceholderParentedThroughCustomHierarchicalReference) {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};
  ASSERT_TRUE(LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(), space,
                                     factory));

  const Node* placeholder = space.GetNode(
      devices::id::Iec60870DeviceType_TransmissionItemPlaceholder);
  ASSERT_TRUE(placeholder);
  const Node* parent = GetParent(*placeholder);
  ASSERT_TRUE(parent);
  EXPECT_EQ(parent->id(), NodeId{devices::id::Iec60870DeviceType});
}

}  // namespace
}  // namespace scada
