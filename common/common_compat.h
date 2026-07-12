#pragma once

// Transitional compatibility shim for the common/common/ namespace migration.
// The `data_services` namespace now nests under `scada`; this using-directive
// lets existing unqualified `data_services::` call sites keep resolving while
// they are migrated incrementally. Remove this header and its includes once
// every consumer has been rewritten to `scada::data_services::`.
//
// Note: `expression` is deliberately NOT nested — it is the third_party/express
// library's namespace (like boost/std), not common's.
namespace scada::data_services {}
namespace data_services {
using namespace scada::data_services;  // NOLINT(build/namespaces) transitional
}  // namespace data_services
