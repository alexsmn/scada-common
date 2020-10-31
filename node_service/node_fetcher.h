#pragma once

#include "common/node_state.h"
#include "core/node_id.h"
#include "core/status.h"

#include <functional>

using NodeFetchStatuses = std::vector<std::pair<scada::NodeId, scada::Status>>;

using NodeValidator = std::function<bool(const scada::NodeId& node_id)>;
using FetchCompletedHandler =
    std::function<void(std::vector<scada::NodeState>&& nodes,
                       NodeFetchStatuses&& errors)>;

class NodeFetcher {
 public:
  virtual ~NodeFetcher() {}

  virtual void Fetch(const scada::NodeId& node_id,
                     bool fetch_parent = false,
                     const scada::NodeId& parent_id = {},
                     const scada::NodeId& reference_type_id = {},
                     bool force = false) = 0;

  virtual void Cancel(const scada::NodeId& node_id) = 0;
};
