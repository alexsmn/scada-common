#pragma once

#include <algorithm>

#include "scada/node_id.h"

namespace scada {

// Non-owning view over the property declaration ids affected by a node
// modification. Must not outlive the id array it points to.
struct PropertyIds {
  PropertyIds(size_t count, const NodeId* ids) : count(count), ids(ids) {}

  bool Has(const NodeId& prop_type_id) const {
    return std::find(ids, ids + count, prop_type_id) != ids + count;
  }

  const NodeId* begin() const { return ids; }
  const NodeId* end() const { return ids + count; }

  size_t count;
  const NodeId* ids;
};

}  // namespace scada
