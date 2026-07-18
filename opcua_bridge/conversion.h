#pragma once

// Bidirectional converters between the SCADA `core` types (`scada::`) and the
// extracted opcuapp types (`opcua::`). The two type universes are
// structurally identical mirrors, so most conversions are mechanical field
// copies. Notably, the std-alias types — `String` (std::string),
// `LocalizedText` (std::u16string), `ByteString` (std::vector<char>) and the
// numeric primitives — are the SAME std type on both sides and need no
// conversion at all; only the class types and `base::Time`-backed `DateTime`
// require real work.

#include "scada/data_value.h"
#include "scada/expanded_node_id.h"
#include "scada/extension_object.h"
#include "scada/node_id.h"
#include "scada/qualified_name.h"
#include "scada/qualifier.h"
#include "scada/status.h"
#include "scada/variant.h"

#include "opcua/types/data_value.h"
#include "opcua/types/expanded_node_id.h"
#include "opcua/types/extension_object.h"
#include "opcua/types/node_id.h"
#include "opcua/types/qualified_name.h"
#include "opcua/types/qualifier.h"
#include "opcua/types/status.h"
#include "opcua/types/variant.h"

#include <limits>
#include <vector>

#include "opcua_bridge/opcua_bridge_compat.h"
namespace scada::opcua_bridge {

// --- enums --------------------------------------------------------------
// Core uses SCADA-internal StatusCode values; opcuapp uses the values that go
// on the wire — the standard OPC UA code where one exists, or a
// vendor-extension SubCode (0x2000+) otherwise. The two enums share
// enumerator names; every shared Bad_* name is listed explicitly (even the
// few whose values coincide) so a new core code can never silently leak its
// internal value onto the wire through the default cast. The Good_* and
// Uncertain_* quality codes intentionally share values on both sides and fall
// through.
#define SCADA_OPCUA_STATUS_CODE_MAP(MAP) \
  MAP(Bad_WrongLoginCredentials)         \
  MAP(Bad_UserIsAlreadyLoggedOn)         \
  MAP(Bad_UnsupportedProtocolVersion)    \
  MAP(Bad_ObjectIsBusy)                  \
  MAP(Bad_WrongNodeId)                   \
  MAP(Bad_WrongDeviceId)                 \
  MAP(Bad_Disconnected)                  \
  MAP(Bad_SessionForcedLogoff)           \
  MAP(Bad_Timeout)                       \
  MAP(Bad_CantDeleteDependentNode)       \
  MAP(Bad_ServerWasShutDown)             \
  MAP(Bad_WrongMethodId)                 \
  MAP(Bad_CantDeleteOwnUser)             \
  MAP(Bad_DuplicateNodeId)               \
  MAP(Bad_UnsupportedFileVersion)        \
  MAP(Bad_WrongTypeId)                   \
  MAP(Bad_WrongParentId)                 \
  MAP(Bad_SessionIsLoggedOff)            \
  MAP(Bad_WrongSubscriptionId)           \
  MAP(Bad_WrongIndex)                    \
  MAP(Bad_Iec60870UnknownType)           \
  MAP(Bad_Iec60870UnknownCot)            \
  MAP(Bad_Iec60870UnknownDevice)         \
  MAP(Bad_Iec60870UnknownAddress)        \
  MAP(Bad_Iec60870UnknownError)          \
  MAP(Bad_WrongCallArguments)            \
  MAP(Bad_CantParseString)               \
  MAP(Bad_TooLongString)                 \
  MAP(Bad_WrongPropertyId)               \
  MAP(Bad_WrongReferenceId)              \
  MAP(Bad_WrongNodeClass)                \
  MAP(Bad_WrongAttributeId)              \
  MAP(Bad_Iec61850Error)                 \
  MAP(Bad_NothingToDo)                   \
  MAP(Bad_BrowseNameInvalid)             \
  MAP(Bad_WrongTargetId)                 \
  MAP(Bad_MonitoredItemIdInvalid)        \
  MAP(Bad_MessageNotAvailable)           \
  MAP(Bad_ApplicationSignatureInvalid)   \
  MAP(Bad_TooManyOperations)             \
  MAP(Bad_TooManyMonitoredItems)         \
  MAP(Bad_SequenceNumberUnknown)         \
  MAP(Bad_NoContinuationPoints)          \
  MAP(Bad_TimestampsToReturnInvalid)     \
  MAP(Bad_ViewIdUnknown)                 \
  MAP(Bad_HistoryOperationInvalid)       \
  MAP(Bad_NoSubscription)                \
  MAP(Bad_UserAccessDenied)              \
  MAP(Bad_NotSupported)

inline opcua::StatusCode ToOpcua(scada::StatusCode c) {
  switch (c) {
#define MAP(name)               \
  case scada::StatusCode::name: \
    return opcua::StatusCode::name;
    SCADA_OPCUA_STATUS_CODE_MAP(MAP)
#undef MAP
    default:
      return static_cast<opcua::StatusCode>(c);
  }
}
inline scada::StatusCode ToScada(opcua::StatusCode c) {
  switch (c) {
#define MAP(name)               \
  case opcua::StatusCode::name: \
    return scada::StatusCode::name;
    SCADA_OPCUA_STATUS_CODE_MAP(MAP)
#undef MAP
    // opcuapp-only code with no core twin: a ServiceFault from a peer maps to
    // the closest core meaning instead of casting its wire value into an
    // unrelated core enumerator.
    case opcua::StatusCode::Bad_ServiceUnsupported:
      return scada::StatusCode::Bad_NotSupported;
    default:
      return static_cast<scada::StatusCode>(c);
  }
}

// --- Status -------------------------------------------------------------
// Preserve only the severity/subcode (mapped) and the limit info bits, which is
// all the codebase models.
inline opcua::Status ToOpcua(scada::Status s) {
  opcua::Status result{ToOpcua(s.code())};
  result.set_limit(
      static_cast<opcua::StatusLimit>(static_cast<int>(s.limit())));
  return result;
}
inline scada::Status ToScada(opcua::Status s) {
  scada::Status result{ToScada(s.code())};
  result.set_limit(
      static_cast<scada::StatusLimit>(static_cast<int>(s.limit())));
  return result;
}

// --- DateTime (base::Time vs opcua::DateTime) -------------------------
inline opcua::DateTime ToOpcua(scada::DateTime t) {
  if (t.is_max())
    return opcua::DateTime::Max();
  if (t.is_min())
    return opcua::DateTime::Min();
  constexpr int64_t kTicksPerMicrosecond =
      opcua::DateTime::kTicksPerMicrosecond;
  const int64_t value = t.ToInternalValue();
  if (value > std::numeric_limits<int64_t>::max() / kTicksPerMicrosecond)
    return opcua::DateTime::Max();
  if (value < std::numeric_limits<int64_t>::min() / kTicksPerMicrosecond)
    return opcua::DateTime::Min();
  return opcua::DateTime::FromInternalValue(value * kTicksPerMicrosecond);
}
inline scada::DateTime ToScada(opcua::DateTime t) {
  if (t.is_max())
    return scada::DateTime::Max();
  if (t.is_min())
    return scada::DateTime::Min();
  return scada::DateTime::FromInternalValue(
      t.ToInternalValue() / opcua::DateTime::kTicksPerMicrosecond);
}

// --- Qualifier ----------------------------------------------------------
inline opcua::Qualifier ToOpcua(scada::Qualifier q) {
  return opcua::Qualifier{q.raw()};
}
inline scada::Qualifier ToScada(opcua::Qualifier q) {
  return scada::Qualifier{q.raw()};
}

// --- class types --------------------------------------------------------
opcua::NodeId ToOpcua(const scada::NodeId&);
scada::NodeId ToScada(const opcua::NodeId&);

opcua::ExpandedNodeId ToOpcua(const scada::ExpandedNodeId&);
scada::ExpandedNodeId ToScada(const opcua::ExpandedNodeId&);

opcua::QualifiedName ToOpcua(const scada::QualifiedName&);
scada::QualifiedName ToScada(const opcua::QualifiedName&);

opcua::ExtensionObject ToOpcua(const scada::ExtensionObject&);
scada::ExtensionObject ToScada(const opcua::ExtensionObject&);

opcua::Variant ToOpcua(const scada::Variant&);
scada::Variant ToScada(const opcua::Variant&);

opcua::DataValue ToOpcua(const scada::DataValue&);
scada::DataValue ToScada(const opcua::DataValue&);

// The element-wise vector helpers (ToOpcuaVector / ToScadaVector) live in
// vector_conversion.h, which both .cpp files include AFTER all ToOpcua/ToScada
// overloads are declared, so ordinary lookup inside the template sees every
// element converter.

}  // namespace scada::opcua_bridge
