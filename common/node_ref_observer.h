#pragma once

#include <cstdint>

#include "core/node_id.h"

struct ModelChangeEvent {
  enum ModelChangeVerb {
    NodeAdded        = 1 << 0,
    NodeDeleted      = 1 << 1,
    ReferenceAdded   = 1 << 2,
    ReferenceDeleted = 1 << 3,
  };

  scada::NodeId node_id;
  scada::NodeId type_definition_id;
  uint8_t verb;
};

class NodeRefObserver {
 public:
  virtual void OnModelChange(const ModelChangeEvent& event) {}
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) {}
};
