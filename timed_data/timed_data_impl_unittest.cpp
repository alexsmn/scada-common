#include "timed_data/timed_data_impl.h"

#include "node_service/static/static_node_service.h"

#include <gmock/gmock.h>

TEST(TimedDataImpl, Test) {
  StaticNodeService node_service;
  auto node = node_service.GetNode(scada::NodeId{1, 1});

  /*auto timed_data = std::make_shared<TimedDataImpl>(node,
  scada::AggregateFilter{}, );*/
}
