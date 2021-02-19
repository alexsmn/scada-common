#pragma once

#include "node_service/node_ref.h"
#include "core/node_id.h"

class NodeRefObserver;

class NodeService {
 public:
  virtual ~NodeService() {}

  virtual NodeRef GetNode(const scada::NodeId& node_id) = 0;

  virtual void Subscribe(NodeRefObserver& observer) const = 0;
  virtual void Unsubscribe(NodeRefObserver& observer) const = 0;

  virtual size_t GetPendingTaskCount() const = 0;
};
