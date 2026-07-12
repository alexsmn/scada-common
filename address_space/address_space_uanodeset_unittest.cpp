// A/B test for the UANodeSet2 loader (Phase 2 of the nodeset migration; see
// common/model/docs/uanodeset-migration.md). Loads the SCADA model two ways --
// from the repo-owned scada-node-state-v1 files and from the UANodeSet2 file
// produced by gen/convert_to_uanodeset.py -- and asserts the resulting address
// spaces are structurally identical (same nodes, classes, names, hierarchy, and
// references). This proves the standard loader is a faithful drop-in.
//
// The UANodeSet2 fixture (model/gen/samples/Scada.Full.NodeSet2.xml) is the
// committed converter output; regenerate it if the nodesets change.

#include "address_space/address_space_xml.h"

#include "address_space/address_space_impl2.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "common/node_state.h"
#include "model/node_id_util.h"
#include "model/static_nodesets.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace scada {
namespace {

// A canonical, order-independent string for one node's structure: everything
// MakeNodeStates reconstructs plus the sorted reference set.
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

std::map<std::string, std::string> Dump(const AddressSpace& address_space) {
  std::map<std::string, std::string> out;
  for (const auto& node_state : MakeNodeStates(address_space)) {
    out.emplace(NodeIdToScadaString(node_state.node_id),
                Canonicalize(node_state));
  }
  return out;
}

std::filesystem::path ModelDir() {
  return GetScadaStaticNodesetSourceDir().parent_path();
}

TEST(UANodeSetLoader, MatchesCustomFormat) {
  // A: the repo-owned scada-node-state-v1 files (the current source of truth).
  AddressSpaceImpl2 custom_space;
  {
    GenericNodeFactory factory{custom_space};
    ASSERT_TRUE(LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(),
                                       custom_space, factory));
  }

  // B: the OPC UA namespace-0 base nodes (still from the custom file, since we
  // only convert the SCADA namespace) plus the SCADA model from the converted
  // UANodeSet2 file. Loaded together so the cross-namespace references resolve.
  AddressSpaceImpl2 ua_space;
  {
    GenericNodeFactory factory{ua_space};
    const std::array<std::filesystem::path, 2> paths = {
        GetScadaStaticNodesetSourceDir() / "opcua_base.xml",
        ModelDir() / "gen" / "samples" / "Scada.Full.NodeSet2.xml"};
    ASSERT_TRUE(LoadStaticNodesets(paths, ua_space, factory));
  }

  const auto a = Dump(custom_space);
  const auto b = Dump(ua_space);

  for (const auto& [id, canonical] : a)
    if (!b.contains(id))
      ADD_FAILURE() << "node in custom but not UANodeSet: " << id << " -> "
                    << canonical;
  for (const auto& [id, canonical] : b)
    if (!a.contains(id))
      ADD_FAILURE() << "node in UANodeSet but not custom: " << id << " -> "
                    << canonical;
  for (const auto& [id, canonical] : a) {
    auto it = b.find(id);
    if (it != b.end())
      EXPECT_EQ(canonical, it->second) << "node " << id << " differs";
  }
}

}  // namespace
}  // namespace scada
