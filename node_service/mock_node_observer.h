#pragma once

#include "node_service/node_observer.h"

#include <gmock/gmock.h>

class MockNodeObserver : public NodeRefObserver {
 public:
  MOCK_METHOD(void,
              OnModelChanged,
              (const scada::ModelChangeEvent& event),
              (override));
  MOCK_METHOD(void,
              OnNodeSemanticChanged,
              (const scada::NodeId& node_id),
              (override));
  MOCK_METHOD(void, OnNodeFetched, (const scada::NodeId& node_id), (override));
};
