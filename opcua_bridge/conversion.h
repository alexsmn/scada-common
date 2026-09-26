#pragma once

// Bidirectional converters between the SCADA `core` types (`scada::`) and the
// extracted opcuapp types (`opcua::`). The two type universes are
// structurally identical mirrors, so most conversions are mechanical field
// copies. Notably, the std-alias types — `String` (std::string),
// `ByteString` (std::vector<char>) and the numeric primitives — are the SAME
// std type on both sides and need no conversion at all; the class types
// (including the `{locale, text}` LocalizedText mirrors) and
// `scada::Time`-backed `Time` require real work.

#include "base/time/time_wire_codec.h"
#include "scada/data_value.h"
#include "scada/expanded_node_id.h"
#include "scada/extension_object.h"
#include "scada/localized_text.h"
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

namespace scada::opcua_bridge {

// --- enums --------------------------------------------------------------
// Core uses SCADA-internal StatusCode values and names; opcuapp uses the
// values that go on the wire — the standard OPC UA code where one exists, or
// a vendor-extension SubCode (0x2000+) otherwise — under the standard OPC UA
// code names. Each MAP entry pairs the core name with its opcuapp twin; every
// Bad code is listed explicitly (even the few whose values coincide) so a new
// core code can never silently leak its internal value onto the wire through
// the default cast. The Good_* and Uncertain_* quality codes intentionally
// share names and values on both sides; they are listed in
// SCADA_OPCUA_STATUS_CODE_SAME below.
//
// Every table here is checked for completeness at compile time: ToOpcua and
// ToScada switch over their enum with no `default:` label, and
// SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_BEGIN turns the compiler's
// unhandled-enumerator diagnostic into an error for that span. So a code added
// to either enum without an entry here fails the build instead of reaching the
// wire as its raw internal value.
//
// A core code whose wire twin is already claimed by an entry here goes in
// SCADA_OPCUA_STATUS_CODE_MAP_TO_WIRE below instead — see the note there.
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
  MAP(Bad_OutOfRange, Bad_OutOfRange)                                   \
  MAP(Bad_NotWritable, Bad_NotWritable)                                 \
  MAP(Bad_ResponseTooLarge, Bad_ResponseTooLarge)                       \
  MAP(Bad_InvalidState, Bad_InvalidState)                               \
  MAP(Bad_NotReadable, Bad_NotReadable)                                 \
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
  MAP(Bad_NotSupported, Bad_NotSupported)                               \
  MAP(Bad_LicenseExpired, Bad_LicenseExpired)                           \
  MAP(Bad_WaitingForInitialData, Bad_WaitingForInitialData)

// Core codes that share a wire code with an entry in the table above.
//
// The two tables exist because the map is consumed in both directions from one
// list, so an opcua name may appear only once — a second entry would be a
// duplicate `case` label in ToScada. Core draws finer distinctions than the
// wire does in a few places (Bad_TooLongString and Bad_OutOfRange are both
// OPC UA's Bad_OutOfRange, "outside the valid range ... or other
// server-defined restrictions", Part 4 §7.38.2), and the distinction is worth
// keeping in logs even though it cannot survive the round trip.
//
// These convert one way only: ToOpcua maps them, and ToScada resolves the wire
// code to whichever core code the main table names. Listing them here rather
// than omitting them is what keeps the default cast from leaking an internal
// enumerator value onto the wire — `Bad | 52` is not a valid StatusCode.
#define SCADA_OPCUA_STATUS_CODE_MAP_TO_WIRE(MAP) \
  MAP(Bad_TooLongString, Bad_OutOfRange)

// Codes that carry the same name and the same value on both sides — the
// severity-only codes and the Good/Uncertain quality codes. They convert by
// value; the static_asserts below keep the "same value" half of that honest.
#define SCADA_OPCUA_STATUS_CODE_SAME(SAME) \
  SAME(Good)                               \
  SAME(Good_Pending)                       \
  SAME(Good_Sporadic)                      \
  SAME(Good_Backup)                        \
  SAME(Good_Manual)                        \
  SAME(Good_Simulated)                     \
  SAME(Uncertain)                          \
  SAME(Uncertain_DeviceFlag)               \
  SAME(Uncertain_Misconfigured)            \
  SAME(Uncertain_Disconnected)             \
  SAME(Uncertain_NotUpdated)               \
  SAME(Uncertain_StateWasNotChanged)       \
  SAME(Bad)

#define SAME(name)                                                  \
  static_assert(static_cast<unsigned>(scada::StatusCode::name) ==   \
                    static_cast<unsigned>(opcua::StatusCode::name), \
                #name                                               \
                " differs between scada:: and opcua::; move it "    \
                "to SCADA_OPCUA_STATUS_CODE_MAP");
SCADA_OPCUA_STATUS_CODE_SAME(SAME)
#undef SAME

// Makes an enumerator missing from a `default:`-less switch a compile error
// between BEGIN and END, whatever the product's warning level. Clang and GCC
// call it -Wswitch; MSVC calls it C4062, which is off by default.
#if defined(__clang__)
#define SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_BEGIN \
  _Pragma("clang diagnostic push")                \
      _Pragma("clang diagnostic error \"-Wswitch\"")
#define SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_END _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_BEGIN \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic error \"-Wswitch\"")
#define SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_END _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#define SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_BEGIN \
  __pragma(warning(push)) __pragma(warning(error : 4062))
#define SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_END __pragma(warning(pop))
#else
#define SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_BEGIN
#define SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_END
#endif

SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_BEGIN

inline opcua::StatusCode ToOpcua(scada::StatusCode c) {
  switch (c) {
#define SAME(name)              \
  case scada::StatusCode::name: \
    return opcua::StatusCode::name;
    SCADA_OPCUA_STATUS_CODE_SAME(SAME)
#undef SAME
#define MAP(scada_name, opcua_name)   \
  case scada::StatusCode::scada_name: \
    return opcua::StatusCode::opcua_name;
    SCADA_OPCUA_STATUS_CODE_MAP(MAP)
    SCADA_OPCUA_STATUS_CODE_MAP_TO_WIRE(MAP)
#undef MAP
  }
  // Not a named enumerator (the switch above names them all, or the build
  // fails): a value that arrived from outside, passed through unchanged.
  return static_cast<opcua::StatusCode>(c);
}
inline scada::StatusCode ToScada(opcua::StatusCode c) {
  switch (c) {
#define SAME(name)              \
  case opcua::StatusCode::name: \
    return scada::StatusCode::name;
    SCADA_OPCUA_STATUS_CODE_SAME(SAME)
#undef SAME
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
  }
  // A wire code opcuapp does not name, e.g. a standard code from a peer.
  return static_cast<scada::StatusCode>(c);
}

SCADA_STATUS_CODE_SWITCH_EXHAUSTIVE_END

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

// --- Time (scada::Time vs opcua::DateTime) -------------------------
// scada::Time is µs since the Unix epoch; opcua::DateTime is 100-ns ticks
// since the Windows 1601 epoch. The µs-since-1601 wire value bridges the two
// (see base/time/time_wire_codec.h), with the range sentinels special-cased.
inline opcua::DateTime ToOpcua(scada::Time t) {
  if (t == scada::kMaxTime)
    return opcua::DateTime::Max();
  if (t == scada::kMinTime)
    return opcua::DateTime::Min();
  constexpr int64_t kTicksPerMicrosecond =
      opcua::DateTime::kTicksPerMicrosecond;
  const int64_t value = scada::base::EncodeWireMicroseconds(t);
  if (value > std::numeric_limits<int64_t>::max() / kTicksPerMicrosecond)
    return opcua::DateTime::Max();
  if (value < std::numeric_limits<int64_t>::min() / kTicksPerMicrosecond)
    return opcua::DateTime::Min();
  return opcua::DateTime::FromInternalValue(value * kTicksPerMicrosecond);
}
inline scada::Time ToScada(opcua::DateTime t) {
  if (t.is_max())
    return scada::kMaxTime;
  if (t.is_min())
    return scada::kMinTime;
  return scada::base::DecodeWireTime(t.ToInternalValue() /
                                     opcua::DateTime::kTicksPerMicrosecond);
}

// --- Qualifier ----------------------------------------------------------
inline opcua::Qualifier ToOpcua(scada::Qualifier q) {
  return opcua::Qualifier{q.raw()};
}
inline scada::Qualifier ToScada(opcua::Qualifier q) {
  return scada::Qualifier{q.raw()};
}

// --- LocalizedText ------------------------------------------------------
// Mirror structs with identical `{locale, text}` layout on both sides.
inline opcua::LocalizedText ToOpcua(const scada::LocalizedText& t) {
  return opcua::LocalizedText{t.locale, t.text};
}
inline scada::LocalizedText ToScada(const opcua::LocalizedText& t) {
  return scada::LocalizedText{t.locale, t.text};
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
