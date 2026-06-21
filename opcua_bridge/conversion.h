#pragma once

// Bidirectional converters between the SCADA `core` types (`scada::`) and the
// extracted opcuapp types (`opcua::scada::`). The two type universes are
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

#include "opcua/scada/data_value.h"
#include "opcua/scada/expanded_node_id.h"
#include "opcua/scada/extension_object.h"
#include "opcua/scada/node_id.h"
#include "opcua/scada/qualified_name.h"
#include "opcua/scada/qualifier.h"
#include "opcua/scada/status.h"
#include "opcua/scada/variant.h"

#include <vector>

namespace opcua_bridge {

// --- enums --------------------------------------------------------------
// Core uses SCADA-internal StatusCode values; opcuapp uses the standard OPC UA
// StatusCode values that go on the wire. The two enums share enumerator names,
// so the boundary maps the codes whose values differ (all others still share a
// value and fall through to a direct cast).
inline opcua::StatusCode ToOpcua(scada::StatusCode c) {
  switch (c) {
#define MAP(name) \
  case scada::StatusCode::name:                \
    return opcua::StatusCode::name;
    MAP(Bad_WrongLoginCredentials)
    MAP(Bad_WrongNodeId)
    MAP(Bad_Timeout)
    MAP(Bad_CantDeleteDependentNode)
    MAP(Bad_WrongMethodId)
    MAP(Bad_UnsupportedFileVersion)
    MAP(Bad_WrongTypeId)
    MAP(Bad_SessionIsLoggedOff)
    MAP(Bad_WrongSubscriptionId)
    MAP(Bad_WrongIndex)
    MAP(Bad_WrongCallArguments)
    MAP(Bad_WrongNodeClass)
    MAP(Bad_WrongAttributeId)
    MAP(Bad_NothingToDo)
    MAP(Bad_BrowseNameInvalid)
    MAP(Bad_MonitoredItemIdInvalid)
    MAP(Bad_MessageNotAvailable)
    MAP(Bad_ApplicationSignatureInvalid)
    MAP(Bad_TooManyOperations)
    MAP(Bad_TooManyMonitoredItems)
    MAP(Bad_SequenceNumberUnknown)
    MAP(Bad_NoContinuationPoints)
    MAP(Bad_TimestampsToReturnInvalid)
    MAP(Bad_ViewIdUnknown)
    MAP(Bad_HistoryOperationInvalid)
    MAP(Bad_NoSubscription)
    MAP(Bad_ServerWasShutDown)
#undef MAP
    default:
      return static_cast<opcua::StatusCode>(c);
  }
}
inline scada::StatusCode ToScada(opcua::StatusCode c) {
  switch (c) {
#define MAP(name) \
  case opcua::StatusCode::name:         \
    return scada::StatusCode::name;
    MAP(Bad_WrongLoginCredentials)
    MAP(Bad_WrongNodeId)
    MAP(Bad_Timeout)
    MAP(Bad_CantDeleteDependentNode)
    MAP(Bad_WrongMethodId)
    MAP(Bad_UnsupportedFileVersion)
    MAP(Bad_WrongTypeId)
    MAP(Bad_SessionIsLoggedOff)
    MAP(Bad_WrongSubscriptionId)
    MAP(Bad_WrongIndex)
    MAP(Bad_WrongCallArguments)
    MAP(Bad_WrongNodeClass)
    MAP(Bad_WrongAttributeId)
    MAP(Bad_NothingToDo)
    MAP(Bad_BrowseNameInvalid)
    MAP(Bad_MonitoredItemIdInvalid)
    MAP(Bad_MessageNotAvailable)
    MAP(Bad_ApplicationSignatureInvalid)
    MAP(Bad_TooManyOperations)
    MAP(Bad_TooManyMonitoredItems)
    MAP(Bad_SequenceNumberUnknown)
    MAP(Bad_NoContinuationPoints)
    MAP(Bad_TimestampsToReturnInvalid)
    MAP(Bad_ViewIdUnknown)
    MAP(Bad_HistoryOperationInvalid)
    MAP(Bad_NoSubscription)
    MAP(Bad_ServerWasShutDown)
#undef MAP
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
  result.set_limit(static_cast<scada::StatusLimit>(static_cast<int>(s.limit())));
  return result;
}

// --- DateTime (base::Time vs opcua::base::Time) -------------------------
inline opcua::DateTime ToOpcua(scada::DateTime t) {
  return opcua::base::Time::FromInternalValue(t.ToInternalValue());
}
inline scada::DateTime ToScada(opcua::DateTime t) {
  return base::Time::FromInternalValue(t.ToInternalValue());
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

}  // namespace opcua_bridge
