#pragma once

#include "core/event.h"
#include "core/node_id.h"

#include <cstdint>

class NodeRefObserver {
 public:
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) {}
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) {}
  virtual void OnNodeFetched(const scada::NodeId& node_id, bool children) {}
};
