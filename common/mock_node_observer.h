#pragma once

#include "common/node_observer.h"

#include <gmock/gmock.h>

class MockNodeObserver : public NodeRefObserver {
 public:
  MOCK_METHOD1(OnModelChanged, void(const scada::ModelChangeEvent& event));
  MOCK_METHOD1(OnNodeSemanticChanged, void(const scada::NodeId& node_id));
  MOCK_METHOD2(OnNodeFetched,
               void(const scada::NodeId& node_id, bool children));
};
