#pragma once

#include "node_service/node_events.h"
#include "node_service/node_ref.h"
#include "scada/node_id.h"

#include <boost/signals2/connection.hpp>

class NodeService {
 public:
  virtual ~NodeService() = default;

  virtual NodeRef GetNode(const scada::NodeId& node_id) = 0;

  // Notifies about node graph changes (nodes/references added or deleted).
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeModelChanged(const ModelChangedCallback& callback) const = 0;
  // Notifies about displayed attribute changes and fetch errors.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const NodeSemanticChangedCallback& callback) const = 0;
  // Notifies when a node fetch completed (successfully or with an error).
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const NodeFetchedCallback& callback) const = 0;
  // Notifies after a node model swapped in a new state snapshot.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(const NodeStateChangedCallback& callback) const = 0;

  virtual size_t GetPendingTaskCount() const = 0;
};
