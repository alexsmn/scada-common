#pragma once

#include <algorithm>

#include "scada/node_id.h"

namespace scada {

class Node;
class ReferenceType;

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

class NodeObserver {
 public:
  virtual void OnNodeCreated(const Node& node) {}
  virtual void OnNodeDeleted(const Node& node) {}
  virtual void OnNodeModified(const Node& node, const PropertyIds& property_ids) {}
  virtual void OnNodeMoved(const Node& node) {}
  virtual void OnNodeTitleChanged(const Node& node) {}
  virtual void OnReferenceAdded(const ReferenceType& reference_type, const Node& source, const Node& target) {}
  virtual void OnReferenceDeleted(const ReferenceType& reference_type, const Node& source, const Node& target) {}
};

} // namespace cfg
