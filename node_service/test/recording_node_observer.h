#pragma once

#include "node_service/node_observer.h"
#include "scada/event.h"

#include <vector>

// Fake NodeRefObserver that records every received notification, so tests
// can assert on observable event sequences instead of encoding mock
// call-shape expectations.
class RecordingNodeObserver : public NodeRefObserver {
 public:
  // NodeRefObserver
  void OnModelChanged(const scada::ModelChangeEvent& event) override {
    model_change_events.push_back(event);
  }

  void OnNodeSemanticChanged(const scada::NodeId& node_id) override {
    semantic_changed_node_ids.push_back(node_id);
  }

  void OnNodeFetched(const NodeFetchedEvent& event) override {
    node_fetched_events.push_back(event);
  }

  void OnNodeStateChanged(const NodeStateChangedEvent& event) override {
    state_changed_events.push_back(event);
  }

  // Latest snapshot published for |node_id|, or nullptr if none was recorded.
  scada::NodeStatePtr last_state(const scada::NodeId& node_id) const {
    for (auto i = state_changed_events.rbegin();
         i != state_changed_events.rend(); ++i) {
      if (i->node_id == node_id)
        return i->state;
    }
    return nullptr;
  }

  // Number of snapshots published for |node_id|.
  size_t state_change_count(const scada::NodeId& node_id) const {
    size_t count = 0;
    for (const auto& event : state_changed_events) {
      if (event.node_id == node_id)
        ++count;
    }
    return count;
  }

  std::vector<scada::ModelChangeEvent> model_change_events;
  std::vector<scada::NodeId> semantic_changed_node_ids;
  std::vector<NodeFetchedEvent> node_fetched_events;
  std::vector<NodeStateChangedEvent> state_changed_events;
};
