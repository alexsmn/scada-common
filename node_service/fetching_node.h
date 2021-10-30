#pragma once

#include "common/node_state.h"
#include "node_service/node_fetch_status.h"

struct FetchingNode {
  bool fetched() const { return attributes_fetched && references_fetched; }

  void ClearDependsOf();
  void ClearDependentNodes();

  scada::NodeState node_state;

  bool pending = false;  // the node is in |pending_queue_|.
  NodeFetchStatus pending_status;
  unsigned pending_sequence = 0;

  // Requests were sent for specified requested status (requests can be already
  // completed).
  NodeFetchStatus fetch_started;

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
