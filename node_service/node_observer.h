#pragma once

#include "scada/node_id.h"
#include "node_service/node_fetch_status.h"

#include <cstdint>

namespace scada {
struct ModelChangeEvent;
}

class NodeRefObserver {
 public:
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) {}

  // Triggered on any displayed attribute change. And on fetch error.
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) {}

  struct NodeFetchedEvent {
    scada::NodeId node_id;
    NodeFetchStatus old_fetch_status;
  };

  virtual void OnNodeFetched(const NodeFetchedEvent& event) {}
};
