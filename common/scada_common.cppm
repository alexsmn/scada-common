// scada.common — named C++20 module facade over the common/common headers.
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md). `export import` of scada.core / scada.metrics
// mirrors scada_common's PUBLIC links.
//
// Deliberately NOT in this facade:
//  - vds_runtime_api.h: extern-C plugin ABI header (macros + C structs);
//    consumers include it directly.
//  - session_proxy_notifier.h: its global-namespace SessionProxyNotifier
//    class template collides by name with core/remote's SessionProxyNotifier
//    class; exporting both would make TUs importing scada.common and
//    scada.remote together ill-formed. Include the header textually.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "common/aggregation.h"
#include "common/aliases.h"
#include "common/audit.h"
#include "common/common_paths.h"
#include "common/coroutine_service_resolver.h"
#include "common/data_services_util.h"
#include "common/data_value_traits.h"
#include "common/file_system.h"
#include "common/format.h"
#include "common/format_helpers.h"
#include "common/formula_util.h"
#include "common/history_util.h"
#include "common/master_data_services.h"
#include "common/node_state.h"
#include "common/node_state_reader.h"
#include "common/node_state_util.h"
#include "common/scada_expression.h"
#include "common/sync_attribute_service.h"
#include "common/sync_node_management_service.h"
#include "common/sync_view_service.h"
#include "common/timed_data_util.h"
#include "common/type_system.h"
#include "common/type_system_util.h"
#include "common/value_format.h"
#include "common/variable_handle.h"

export module scada.common;

// Mirror scada_common's PUBLIC link transitivity.
export import scada.core;
export import scada.metrics;

// Keep the GMF-attached TimedDataTraits<scada::DataValue> specialization
// (data_value_traits.h) decl-reachable for import-only TUs.
static_assert(sizeof(TimedDataTraits<scada::DataValue>) > 0);

export namespace scada {

// aggregation.h
using scada::AggregateState;
using scada::Aggregator;
using scada::GetAggregateInterval;
using scada::GetAggregator;
using scada::GetLocalAggregateStartTime;

// common_paths.h (unnamed-enum path keys + registration)
using scada::PATH_END;
using scada::PATH_START;
using scada::RegisterPathProvider;

// node_state.h / node_state_util.h
using scada::FindProperty;
using scada::FindReference;
using scada::FindReferenceTarget;
using scada::GetProperty;
using scada::NodeProperties;
using scada::NodeProperty;
using scada::NodeState;
using scada::NodeStatePtr;
using scada::ReadAttribute;
using scada::ReferenceState;
using scada::SetOrAddOrDeleteProperty;
using scada::SetProperties;
using scada::SetProperty;
using scada::SetReference;
using scada::SetReferences;
using scada::SortNodesHierarchically;

// variable_handle.h
using scada::CreateMonitoredVariable;
using scada::VariableHandle;
using scada::VariableHandleImpl;

// Free operator sets at scada namespace scope across the included headers
// (e.g. operator<< for NodeState).
using scada::operator<<;

namespace service_resolver {
// coroutine_service_resolver.h
using scada::service_resolver::RequireCoroutineService;
using scada::service_resolver::RequireSharedService;
using scada::service_resolver::ResolveCoroutineService;
using scada::service_resolver::ResolveCoroutineServiceShared;
}  // namespace service_resolver

}  // namespace scada

export namespace scada::data_services {

// data_services_util.h
using scada::data_services::FromUnownedServices;
using scada::data_services::HasServices;
using scada::data_services::Unowned;

}  // namespace scada::data_services

export {
  // aliases.h / audit.h
  using ::AliasResolveCallback;
  using ::AliasResolver;
  using ::Audit;
  using ::AuditContext;
  using ::AuditDataServices;
  using ::AuditScadaServices;

  // file_system.h (kMaxFileSize is a plain const => internal linkage,
  // include-only)
  using ::CalculateFileHash;

  // format.h / format_helpers.h (Format extends the ::Format overload set
  // exported by scada.base)
  using ::EscapeColoredString;
  using ::Format;
  using ::FormatFloat;
  using ::FormatTit;
  using ::FormatTs;
  using ::StringToValue;
  using ::TitFormatParams;
  using ::TsFormatParams;

  // formula_util.h
  using ::GetFormulaSingleName;
  using ::GetFormulaSingleNodeId;
  using ::GetParentGroupChannelPath;
  using ::MakeNodeIdFormula;

  // history_util.h
  using ::CancelHistory;
  using ::ScopedContinuationPoint;

  // master_data_services.h / node_state_reader.h / scada_expression.h
  using ::MasterDataServices;
  using ::NodeStateReader;
  using ::NodeStateReader2;
  using ::ScadaExpression;

  // sync_*.h
  using ::Browse;
  using ::Read;
  using ::SyncAttributeService;
  using ::SyncNodeManagementService;
  using ::SyncViewService;

  // timed_data_util.h
  using ::FindInsertPosition;
  using ::GetValueAt;
  using ::IsReverseTimeSorted;
  using ::IsTimeSorted;
  using ::LowerBound;
  using ::ReplaceSubrange;
  using ::ReverseUpperBound;
  using ::UpperBound;

  // type_system.h / type_system_util.h
  using ::MightWantReferenceSubtype;
  using ::TypeSystem;
  using ::WantsBrowseDirection;
  using ::WantsOrganizes;
  using ::WantsParent;
  using ::WantsReference;
  using ::WantsReferenceOfSupertype;
  using ::WantsTypeDefinition;

  // value_format.h
  using ::FORMAT_COLOR;
  using ::FORMAT_DEFAULT;
  using ::FORMAT_QUALITY;
  using ::FORMAT_STATUS;
  using ::FORMAT_UNITS;
  using ::FormatFlags;
  using ::ValueFormat;
}  // export
