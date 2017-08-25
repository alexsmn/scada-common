#pragma once

#include "common/node_ref.h"

#include <map>

class NodeRefCache {
 public:
  void Add(NodeRef node);
  void Remove(const scada::NodeId& node_id);

  NodeRef Find(const scada::NodeId& node_id) const;

 private:
  std::map<scada::NodeId, NodeRef> nodes_;
};
