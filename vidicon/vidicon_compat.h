#pragma once

// Transitional compatibility shim for the `vidicon::` -> `scada::vidicon::`
// migration. common/vidicon/ now declares its symbols in the nested
// `scada::vidicon` namespace; this using-directive lets existing unqualified
// `vidicon::` call sites (in common, server, and client) keep resolving while
// they are migrated incrementally. Remove this header and its includes once
// every consumer has been rewritten to `scada::vidicon::`.
namespace scada::vidicon {}
namespace vidicon {
using namespace scada::vidicon;  // NOLINT(build/namespaces) transitional shim
}  // namespace vidicon
