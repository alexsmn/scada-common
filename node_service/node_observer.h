#pragma once

#include "core/node_id.h"

#include <cstdint>

namespace scada {
struct ModelChangeEvent;
}

class NodeRefObserver {
 public:
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) {}
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) {}
  virtual void OnNodeFetched(const scada::NodeId& node_id, bool children) {}
};
