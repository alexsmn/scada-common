#pragma once

#include "common/node_ref.h"
#include "core/status.h"

class NodeRefObserver;

class NodeService {
 public:
  virtual ~NodeService() {}

  virtual NodeRef GetNode(const scada::NodeId& node_id) = 0;

  virtual void AddObserver(NodeRefObserver& observer) = 0;
  virtual void RemoveObserver(NodeRefObserver& observer) = 0;
};
