#pragma once

// Transitional compatibility shim for the `opc::` -> `scada::opc::` migration.
// common/opc/ now declares its converters in the nested `scada::opc` namespace
// (which core already established for its opc node-ids). This using-directive
// lets existing unqualified `opc::` call sites keep resolving while they are
// migrated incrementally. Remove this header and its includes once every
// consumer has been rewritten to `scada::opc::`.
namespace scada::opc {}
namespace opc {
using namespace scada::opc;  // NOLINT(build/namespaces) transitional shim
}  // namespace opc
