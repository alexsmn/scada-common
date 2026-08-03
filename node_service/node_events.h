#pragma once

#include "common/node_state.h"
#include "node_service/node_fetch_status.h"
#include "scada/node_id.h"

#include <boost/signals2/signal.hpp>
#include <functional>

namespace scada {
struct ModelChangeEvent;
}

// Fired when a node fetch completed (successfully or with an error).
struct NodeFetchedEvent {
  scada::NodeId node_id;
  NodeFetchStatus old_fetch_status;
};

// Fired after the model swapped in a new snapshot: on first fetch, on any
// attribute/property/reference change, and on children-fetch completion.
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

using ModelChangedCallback =
    std::function<void(const scada::ModelChangeEvent& event)>;
// Triggered on any displayed attribute change. And on fetch error.
using NodeSemanticChangedCallback =
    std::function<void(const scada::NodeId& node_id)>;
using NodeFetchedCallback = std::function<void(const NodeFetchedEvent& event)>;
using NodeStateChangedCallback =
    std::function<void(const NodeStateChangedEvent& event)>;

// Bundles the four node-event signals emitted by NodeService/NodeModel
// implementations. Fields are mutable so implementations can connect and emit
// from const subscribe/notify paths (the interfaces expose const
// Subscribe* methods).
struct NodeSignals {
  mutable boost::signals2::signal<void(const scada::ModelChangeEvent&)>
      model_changed;
  mutable boost::signals2::signal<void(const scada::NodeId&)>
      node_semantic_changed;
  mutable boost::signals2::signal<void(const NodeFetchedEvent&)> node_fetched;
  mutable boost::signals2::signal<void(const NodeStateChangedEvent&)>
      node_state_changed;
};
