#pragma once

#include "common/node_state.h"
#include "core/node_id.h"

#include <functional>
#include <optional>

using NodeFetchStatuses = std::vector<std::pair<scada::NodeId, scada::Status>>;

using NodeValidator = std::function<bool(const scada::NodeId& node_id)>;
using FetchCompletedHandler =
    std::function<void(std::vector<scada::NodeState>&& nodes,
                       NodeFetchStatuses&& errors)>;

class NodeFetcher {
 public:
  virtual ~NodeFetcher() {}

  virtual void Fetch(const scada::NodeId& node_id, bool force = false) = 0;

  virtual void Cancel(const scada::NodeId& node_id) = 0;

  virtual size_t GetPendingNodeCount() const = 0;
};
