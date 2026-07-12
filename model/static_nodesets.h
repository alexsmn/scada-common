#pragma once

#include <array>
#include <filesystem>
#include <string_view>
#include <vector>

namespace scada {

// The SCADA static address space is the single source of truth for the standard
// OPC UA base nodes plus the SCADA type system. It is partitioned into one file
// per namespace / protocol module under `server/base/nodesets/`. The files are
// listed in dependency-sensible load order, though `LoadStaticAddressSpace`
// resolves references across files regardless of order.
inline constexpr std::array<std::string_view, 2> kScadaStaticNodesetFiles = {
    // OPC UA namespace-0 base types (repo-owned scada-node-state-v1 subset; the
    // official Opc.Ua.NodeSet2.xml is a later migration step) followed by the
    // SCADA model as a standard OPC UA UANodeSet2. LoadStaticAddressSpace
    // detects each file's format; the base must precede the SCADA model so its
    // types resolve. See common/model/docs/uanodeset-migration.md.
    "opcua_base.xml",
    "Scada.NodeSet2.xml",
};

// Absolute path to the committed nodeset source directory in the server source
// tree (resolved via the translation unit location). Intended for tests, tools,
// and build steps that copy the files into a deployment data directory. Runtime
// server code should resolve the files from its own data directory instead.
std::filesystem::path GetScadaStaticNodesetSourceDir();

// Source-tree paths of all partition files in load order. Convenience wrapper
// over `GetScadaStaticNodesetSourceDir()` and `kScadaStaticNodesetFiles`.
std::vector<std::filesystem::path> GetScadaStaticNodesetSourcePaths();

}  // namespace scada
