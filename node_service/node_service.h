#pragma once

#include "scada/node_id.h"
#include "node_service/node_ref.h"

class NodeRefObserver;

class NodeService {
 public:
  virtual ~NodeService() = default;

  virtual NodeRef GetNode(const scada::NodeId& node_id) = 0;

  virtual void Subscribe(NodeRefObserver& observer) const = 0;
  virtual void Unsubscribe(NodeRefObserver& observer) const = 0;

  virtual size_t GetPendingTaskCount() const = 0;
};
