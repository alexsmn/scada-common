#include "timed_data/timed_data_impl.h"

#include "node_service/test/test_node_model.h"

#include <gmock/gmock.h>

TEST(TimedDataImpl, Test) {
  TestNodeService node_service;
  auto node = node_service.GetNode(scada::NodeId{1, 1});

  /*auto timed_data = std::make_shared<TimedDataImpl>(node,
  scada::AggregateFilter{}, );*/
}
