#include "timed_data/timed_data_impl.h"

#include "node_service/test/test_node_model.h"

#include <gmock/gmock.h>

TEST(TimedDataImpl, Test) {
  NodeRef node{std::make_shared<TestNodeModel>()};

  /*auto timed_data = std::make_shared<TimedDataImpl>(node,
  scada::AggregateFilter{}, );*/
}
