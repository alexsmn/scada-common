#pragma once

#include "core/node_id.h"

#include <map>
#include <memory>

class NodeRefImpl;

class NodeRefCache {
 public:
  void Add(std::shared_ptr<NodeRefImpl> node);
  void Remove(const scada::NodeId& node_id);

  std::shared_ptr<NodeRefImpl> Find(const scada::NodeId& node_id) const;

 private:
  std::map<scada::NodeId, std::shared_ptr<NodeRefImpl>> nodes_;
};
