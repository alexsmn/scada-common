#pragma once

#include <cstdint>

#include "core/model_change_event.h"
#include "core/node_id.h"

class NodeRefObserver {
 public:
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) {}
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) {}
  virtual void OnNodeFetched(const scada::NodeId& node_id, bool children) {}
};
