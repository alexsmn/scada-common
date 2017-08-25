#pragma once

#include <algorithm>
#include <vector>

#include "core/node_id.h"

namespace scada {

class Node;
class ReferenceType;

using PropertyIds = std::vector<NodeId>;

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
