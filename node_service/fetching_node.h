#pragma once

#include "common/node_state.h"

struct FetchingNode : scada::NodeState {
  bool fetched() const { return attributes_fetched && references_fetched; }

  void ClearDependsOf();
  void ClearDependentNodes();

  bool pending = false;  // the node is in |pending_queue_|.
  unsigned pending_sequence = 0;

  bool fetch_started = false;     // requests were sent (can be completed).
  unsigned fetch_request_id = 0;  // for request cancelation
  std::vector<FetchingNode*> depends_of;
  std::vector<FetchingNode*> dependent_nodes;
  bool attributes_fetched = false;
  bool references_fetched = false;
  scada::Status status{scada::StatusCode::Good};
  bool force = false;  // refetch even if fetching has been already started
  bool is_property = false;  // instance or type declaration property
  bool is_declaration = false;

  int fetch_cache_iteration = 0;
  int fetch_cache_state = false;
};
