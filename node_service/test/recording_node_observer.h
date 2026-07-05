#pragma once

#include "node_service/node_events.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/event.h"

#include <boost/signals2/connection.hpp>
#include <vector>

// Fake node-event subscriber that records every received notification, so
// tests can assert on observable event sequences instead of encoding mock
// call-shape expectations. Connect it to a NodeService, NodeModel, or NodeRef
// via one of the Connect overloads.
class RecordingNodeObserver {
 public:
  // Connects to all four node-event signals of |source|.
  template <class Source>
  void Connect(const Source& source) {
    connections_.push_back(source.SubscribeModelChanged(
        [this](const scada::ModelChangeEvent& event) {
          model_change_events.push_back(event);
        }));
    connections_.push_back(source.SubscribeNodeSemanticChanged(
        [this](const scada::NodeId& node_id) {
          semantic_changed_node_ids.push_back(node_id);
        }));
    connections_.push_back(
        source.SubscribeNodeFetched([this](const NodeFetchedEvent& event) {
          node_fetched_events.push_back(event);
        }));
    connections_.push_back(source.SubscribeNodeStateChanged(
        [this](const NodeStateChangedEvent& event) {
          state_changed_events.push_back(event);
        }));
  }

  void Disconnect() { connections_.clear(); }

  // Drops all recorded events; connections stay alive.
  void ClearEvents() {
    model_change_events.clear();
    semantic_changed_node_ids.clear();
    node_fetched_events.clear();
    state_changed_events.clear();
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

 private:
  std::vector<boost::signals2::scoped_connection> connections_;
};
