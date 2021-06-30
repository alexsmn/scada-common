#include "node_service/v1/node_fetch_status_tracker.h"

#include <gmock/gmock.h>

#include "address_space/address_space_impl.h"
#include "address_space/object.h"
#include "base/logger.h"

using namespace testing;

namespace v1 {

class NodeFetchStatusTrackerTest : public Test {
 protected:
  void AddTestNode(const scada::NodeId& node_id);

  MockFunction<void(base::span<const NodeFetchStatusChangedItem> items)>
      node_fetch_status_changed_handler_;

  MockFunction<bool(const scada::NodeId& node_id)> node_validator_;

  AddressSpaceImpl address_space_{std::make_shared<NullLogger>()};

  NodeFetchStatusTracker node_fetch_status_tracker_{
      {node_fetch_status_changed_handler_.AsStdFunction(),
       node_validator_.AsStdFunction(), address_space_}};
};

void NodeFetchStatusTrackerTest::AddTestNode(const scada::NodeId& node_id) {
  address_space_.AddStaticNode<scada::GenericObject>(node_id, "Node1BrowseName",
                                                     L"Node1DisplayName");
}

TEST_F(NodeFetchStatusTrackerTest, OnNodesFetched_GoodStatus) {
  const scada::NodeId node_id{1, 1};
  const scada::Status good_status{scada::StatusCode::Good};

  // For good statuses the node must exist in the address space.
  AddTestNode(node_id);

  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(ElementsAre(NodeFetchStatusChangedItem{
                  node_id, good_status, NodeFetchStatus::NodeOnly()})));

  node_fetch_status_tracker_.OnNodesFetched({{node_id, good_status}});
}

TEST_F(NodeFetchStatusTrackerTest, OnNodesFetched_BadStatus) {
  const scada::NodeId node_id{1, 1};
  const scada::Status bad_status{scada::StatusCode::Bad_WrongNodeId};

  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(ElementsAre(NodeFetchStatusChangedItem{
                  node_id, bad_status, NodeFetchStatus::NodeOnly()})));

  node_fetch_status_tracker_.OnNodesFetched({{node_id, bad_status}});
}

}  // namespace v1
