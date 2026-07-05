#pragma once

#include "node_service/node_service.h"

#include <gmock/gmock.h>

class MockNodeService : public NodeService {
 public:
  MOCK_METHOD(NodeRef, GetNode, (const scada::NodeId& node_id));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeModelChanged,
              (const ModelChangedCallback& callback),
              (const));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeNodeSemanticChanged,
              (const NodeSemanticChangedCallback& callback),
              (const));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeNodeFetched,
              (const NodeFetchedCallback& callback),
              (const));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeNodeStateChanged,
              (const NodeStateChangedCallback& callback),
              (const));
  MOCK_METHOD(size_t, GetPendingTaskCount, (), (const));
};
