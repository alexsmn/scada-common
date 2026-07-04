#pragma once

#include "common/node_state.h"
#include "node_service/node_fetch_status.h"
#include "scada/node_id.h"

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

  struct NodeStateChangedEvent {
    scada::NodeId node_id;
    // The newly published snapshot; never null and never mutated after
    // publishing, so consumers can keep it and use it directly instead of
    // re-getting the node and re-reading its attributes.
    scada::NodeStatePtr state;
    // Fetch level the snapshot satisfies (e.g. whether child references are
    // included yet).
    NodeFetchStatus fetch_status;
  };

  // Fired after the model swapped in a new snapshot: on first fetch, on any
  // attribute/property/reference change, and on children-fetch completion.
  virtual void OnNodeStateChanged(const NodeStateChangedEvent& event) {}
};
