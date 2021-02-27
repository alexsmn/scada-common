#pragma once

#include "node_service/node_service.h"

#include <gmock/gmock.h>

class MockNodeService : public NodeService {
 public:
  MOCK_METHOD(NodeRef, GetNode, (const scada::NodeId& node_id));
  MOCK_METHOD(void, Subscribe, (NodeRefObserver & observer), (const));
  MOCK_METHOD(void, Unsubscribe, (NodeRefObserver & observer), (const));
  MOCK_METHOD(size_t, GetPendingTaskCount, (), (const));
};
