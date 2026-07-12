#pragma once

// Transitional compatibility shim for the `core/model/` namespace migration.
// The node-id and static-type vocabularies that `core/model/` contributes to
// the shared domain namespaces (opc, data_items, security, devices, history,
// filesystem) plus the model-owned cfg / NamespaceIndexes namespaces now nest
// under `scada`. core/model only ever adds the `id` / `numeric_id`
// sub-namespaces to the shared domains, so these using-directives redirect the
// node-id lookups (e.g. `devices::id::Foo`) to their new `scada::devices::id`
// home without disturbing the domain classes that server/common define directly
// in `devices::` etc. Remove this header and its includes once every consumer
// has been rewritten to the `scada::` form.
namespace scada::opc {}
namespace opc {
using namespace scada::opc;  // NOLINT(build/namespaces) transitional shim
}
namespace scada::data_items {}
namespace data_items {
using namespace scada::data_items;  // NOLINT(build/namespaces) transitional
}
namespace scada::security {}
namespace security {
using namespace scada::security;  // NOLINT(build/namespaces) transitional shim
}
namespace scada::devices {}
namespace devices {
using namespace scada::devices;  // NOLINT(build/namespaces) transitional shim
}
namespace scada::history {}
namespace history {
using namespace scada::history;  // NOLINT(build/namespaces) transitional shim
}
namespace scada::filesystem {}
namespace filesystem {
using namespace scada::filesystem;  // NOLINT(build/namespaces) transitional
}
namespace scada::cfg {}
namespace cfg {
using namespace scada::cfg;  // NOLINT(build/namespaces) transitional shim
}
namespace scada::NamespaceIndexes {}
namespace NamespaceIndexes {
using namespace scada::NamespaceIndexes;  // NOLINT(build/namespaces) shim
}
