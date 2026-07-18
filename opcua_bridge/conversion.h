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
// Core uses SCADA-internal StatusCode values and names; opcuapp uses the
// values that go on the wire — the standard OPC UA code where one exists, or
// a vendor-extension SubCode (0x2000+) otherwise — under the standard OPC UA
// code names. Each MAP entry pairs the core name with its opcuapp twin; every
// Bad code is listed explicitly (even the few whose values coincide) so a new
// core code can never silently leak its internal value onto the wire through
// the default cast. The Good_* and Uncertain_* quality codes intentionally
// share names and values on both sides and fall through.
#define SCADA_OPCUA_STATUS_CODE_MAP(MAP)                                \
  MAP(Bad_WrongLoginCredentials, Bad_IdentityTokenRejected)             \
  MAP(Bad_UserIsAlreadyLoggedOn, Bad_UserIsAlreadyLoggedOn)             \
  MAP(Bad_UnsupportedProtocolVersion, Bad_ProtocolVersionUnsupported)   \
  MAP(Bad_ObjectIsBusy, Bad_ResourceUnavailable)                        \
  MAP(Bad_WrongNodeId, Bad_NodeIdUnknown)                               \
  MAP(Bad_WrongDeviceId, Bad_WrongDeviceId)                             \
  MAP(Bad_Disconnected, Bad_NoCommunication)                            \
  MAP(Bad_SessionForcedLogoff, Bad_SessionClosed)                       \
  MAP(Bad_Timeout, Bad_Timeout)                                         \
  MAP(Bad_CantDeleteDependentNode, Bad_CantDeleteDependentNode)         \
  MAP(Bad_ServerWasShutDown, Bad_Shutdown)                              \
  MAP(Bad_WrongMethodId, Bad_MethodInvalid)                             \
  MAP(Bad_CantDeleteOwnUser, Bad_CantDeleteOwnUser)                     \
  MAP(Bad_DuplicateNodeId, Bad_NodeIdExists)                            \
  MAP(Bad_UnsupportedFileVersion, Bad_UnsupportedFileVersion)           \
  MAP(Bad_WrongTypeId, Bad_TypeDefinitionInvalid)                       \
  MAP(Bad_WrongParentId, Bad_ParentNodeIdInvalid)                       \
  MAP(Bad_SessionIsLoggedOff, Bad_SessionIdInvalid)                     \
  MAP(Bad_WrongSubscriptionId, Bad_SubscriptionIdInvalid)               \
  MAP(Bad_WrongIndex, Bad_ContinuationPointInvalid)                     \
  MAP(Bad_Iec60870UnknownType, Bad_Iec60870UnknownType)                 \
  MAP(Bad_Iec60870UnknownCot, Bad_Iec60870UnknownCot)                   \
  MAP(Bad_Iec60870UnknownDevice, Bad_Iec60870UnknownDevice)             \
  MAP(Bad_Iec60870UnknownAddress, Bad_Iec60870UnknownAddress)           \
  MAP(Bad_Iec60870UnknownError, Bad_Iec60870UnknownError)               \
  MAP(Bad_WrongCallArguments, Bad_InvalidArgument)                      \
  MAP(Bad_CantParseString, Bad_TypeMismatch)                            \
  MAP(Bad_TooLongString, Bad_OutOfRange)                                \
  MAP(Bad_WrongPropertyId, Bad_WrongPropertyId)                         \
  MAP(Bad_WrongReferenceId, Bad_ReferenceTypeIdInvalid)                 \
  MAP(Bad_WrongNodeClass, Bad_NodeClassInvalid)                         \
  MAP(Bad_WrongAttributeId, Bad_AttributeIdInvalid)                     \
  MAP(Bad_Iec61850Error, Bad_Iec61850Error)                             \
  MAP(Bad_NothingToDo, Bad_NothingToDo)                                 \
  MAP(Bad_BrowseNameInvalid, Bad_BrowseNameInvalid)                     \
  MAP(Bad_WrongTargetId, Bad_TargetNodeIdInvalid)                       \
  MAP(Bad_MonitoredItemIdInvalid, Bad_MonitoredItemIdInvalid)           \
  MAP(Bad_MessageNotAvailable, Bad_MessageNotAvailable)                 \
  MAP(Bad_ApplicationSignatureInvalid, Bad_ApplicationSignatureInvalid) \
  MAP(Bad_TooManyOperations, Bad_TooManyOperations)                     \
  MAP(Bad_TooManyMonitoredItems, Bad_TooManyMonitoredItems)             \
  MAP(Bad_SequenceNumberUnknown, Bad_SequenceNumberUnknown)             \
  MAP(Bad_NoContinuationPoints, Bad_NoContinuationPoints)               \
  MAP(Bad_TimestampsToReturnInvalid, Bad_TimestampsToReturnInvalid)     \
  MAP(Bad_ViewIdUnknown, Bad_ViewIdUnknown)                             \
  MAP(Bad_HistoryOperationInvalid, Bad_HistoryOperationInvalid)         \
  MAP(Bad_NoSubscription, Bad_NoSubscription)                           \
  MAP(Bad_UserAccessDenied, Bad_UserAccessDenied)                       \
  MAP(Bad_NotSupported, Bad_NotSupported)

inline opcua::StatusCode ToOpcua(scada::StatusCode c) {
  switch (c) {
#define MAP(scada_name, opcua_name)   \
  case scada::StatusCode::scada_name: \
    return opcua::StatusCode::opcua_name;
    SCADA_OPCUA_STATUS_CODE_MAP(MAP)
#undef MAP
    default:
      return static_cast<opcua::StatusCode>(c);
  }
}
inline scada::StatusCode ToScada(opcua::StatusCode c) {
  switch (c) {
#define MAP(scada_name, opcua_name)   \
  case opcua::StatusCode::opcua_name: \
    return scada::StatusCode::scada_name;
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
