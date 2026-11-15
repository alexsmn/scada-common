#pragma once

// The canonical NamespaceArray: the URI of every namespace the model defines,
// indexed by its canonical NamespaceIndex (namespaces.csv / namespaces.h).
//
// This is the local index space a process uses in memory. It is deliberately
// separate from the config-table metadata: an OPC UA client needs it to map a
// server's published indexes onto its own compiled-in NamespaceIndexes::
// constants, and a client has no business linking config tables. The
// per-tier served subset, which IS config-table driven, lives server-side.
// See docs/adr/0003-tier-owned-namespace-arrays.md.

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace scada::model {

// The OPC UA standard namespace URI, always NamespaceIndex 0.
inline constexpr std::string_view kOpcUaNamespaceUri =
    "http://opcfoundation.org/UA/";

// uris[i] is the URI at canonical NamespaceIndex i. Every entry is a real,
// unique URI: OPC UA Part 3 §8.2.2 NamespaceIndex defines NamespaceArray
// entries as namespace URIs, and a table that merges by URI equality would
// collapse two empty entries onto one index.
// https://reference.opcfoundation.org/Core/Part3/v105/docs/8.2.2
std::vector<std::string> GetCanonicalNamespaceUris();

// The namespace every Telecontrol SCADA nodeset declares
// (common/model/nodesets/*.xml), canonical index NamespaceIndexes::SCADA. A
// server that publishes it in its NamespaceArray is one of ours.
inline constexpr std::string_view kScadaNamespaceUri =
    "http://telecontrol.ru/opcua/scada";

// Whose server a session is connected to.
enum class ServerKind {
  // A Telecontrol SCADA server (any tier, any version): it publishes
  // kScadaNamespaceUri, so the SCADA-only surfaces have something to show.
  kOurs,
  // Any other OPC UA server. Only the generic surfaces apply: browsing from
  // RootFolder, reading, writing and subscribing to Variables.
  kForeign,
};

// Classifies a server by the NamespaceArray it publishes
// (Server_NamespaceArray, i=2255). The URI, not an index, is the namespace's
// identity: OPC UA Part 3 §8.2.2 NamespaceIndex,
// https://reference.opcfoundation.org/Core/Part3/v105/docs/8.2.2 .
// docs/product/multi-server-and-distribution.md, "foreign server" item 1.
ServerKind ClassifyServer(std::span<const std::string> server_namespace_uris);

}  // namespace scada::model
