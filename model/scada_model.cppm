// scada.model — named C++20 module facade over the core/model headers.
//
// Same design and rules as scada.base (see base/scada_base.cppm and
// docs/cxx-modules.md). `export import scada.core;` mirrors scada_model's
// PUBLIC link on scada_core.
//
// Deliberately NOT in this facade: the node-id constant headers
// (model/*_node_ids.h: scada_node_ids.h, devices_node_ids.h,
// data_items_node_ids.h, history_node_ids.h, security_node_ids.h,
// filesystem_node_ids.h, opc_node_ids.h) and the NamespaceIndexes::* /
// scada::id::* constants. They are namespace-scope non-inline `const` /
// `constexpr` objects with INTERNAL linkage - ill-formed to export and
// pure BMI bloat to include. Importing TUs #include those headers
// textually (before the import), exactly like macros.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "model/namespaces.h"
#include "model/nested_node_ids.h"
#include "model/node_id_util.h"
#include "model/static_nodesets.h"
#include "model/static_types.h"

export module scada.model;

export import scada.core;

export {
  // node_id_util.h / nested_node_ids.h
  using ::GetNestedSubName;
  using ::GetRootNestedNodeId;
  using ::IsNestedNodeId;
  using ::MakeNestedNodeId;
  using ::NestedNodeId;
  using ::NodeIdFromScadaString;
  using ::NodeIdToScadaString;

  // namespaces.h (functions only; the NamespaceIndexes::* constants have
  // internal linkage and stay include-only)
  using ::FindNamespaceIndexByName;
  using ::GetNamespaceName;
}  // export

export namespace scada {

// static_nodesets.h (kScadaStaticNodesetFiles is `inline constexpr` —
// external linkage, exportable; the node-id constant headers stay excluded)
using scada::GetScadaStaticNodesetSourceDir;
using scada::GetScadaStaticNodesetSourcePaths;
using scada::kScadaStaticNodesetFiles;

}  // namespace scada

export namespace scada::cfg {

// static_types.h (the inline constants have external linkage - exportable)
using scada::cfg::Iec60870Mode;
using scada::cfg::Iec60870Protocol;
using scada::cfg::kDataGroupDevicePlaceholder;
using scada::cfg::ModbusEncoding;
using scada::cfg::ModbusMode;
using scada::cfg::NUM_CHANNELS;
using scada::cfg::SimulationSignalType;

}  // namespace scada::cfg
