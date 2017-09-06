#include "node_ref_cache.h"

#include "node_ref_impl.h"

void NodeRefCache::Add(std::shared_ptr<NodeRefImpl> node) {
  nodes_.emplace(node->id(), std::move(node));
}

void NodeRefCache::Remove(const scada::NodeId& node_id) {
  nodes_.erase(node_id);
}

std::shared_ptr<NodeRefImpl> NodeRefCache::Find(const scada::NodeId& node_id) const {
  auto i = nodes_.find(node_id);
  return i == nodes_.end() ? nullptr : i->second;
}
