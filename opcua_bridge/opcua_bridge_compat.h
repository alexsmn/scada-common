#pragma once

// Transitional compatibility shim for the `opcua_bridge::` ->
// `scada::opcua_bridge::` migration. common/opcua_bridge/ now declares its
// symbols in the nested `scada::opcua_bridge` namespace; this using-directive
// lets existing unqualified `opcua_bridge::` call sites keep resolving while
// they are migrated incrementally. Remove this header and its includes once
// every consumer has been rewritten to `scada::opcua_bridge::`.
//
// Note: the namespace is deliberately `scada::opcua_bridge`, NOT `scada::opcua`
// — the bridge references opcuapp's third-party `::opcua` types pervasively, and
// a `scada::opcua` would shadow them.
namespace scada::opcua_bridge {}
namespace opcua_bridge {
using namespace scada::opcua_bridge;  // NOLINT(build/namespaces) transitional
}  // namespace opcua_bridge
