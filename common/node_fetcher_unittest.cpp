#include "node_fetcher.h"

#include "base/logger.h"
#include "common/node_id_util.h"
#include "core/attribute_service_mock.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/test/test_address_space.h"
#include "core/view_service_mock.h"

using namespace testing;

namespace {

struct TestContext {
  MOCK_METHOD1(OnFetched, void(std::vector<scada::NodeState>& nodes));

  void ProcessFetchedNodes(const std::vector<scada::NodeState>& nodes) {
    for (auto& node : nodes)
      fetched_nodes_[node.node_id] = node;
  }

  NullLogger logger;

  TestAddressSpace address_space;

  NodeFetcher node_fetcher{NodeFetcherContext{
      logger,
      address_space,
      address_space,
      [&](std::vector<scada::NodeState>&& nodes, NodeFetchErrors&& errors) {
        OnFetched(std::move(nodes));
        ProcessFetchedNodes(nodes);
      },
      [&](const scada::NodeId& node_id) {
        return fetched_nodes_.find(node_id) != fetched_nodes_.end();
      },
  }};

  std::map<scada::NodeId, scada::NodeState> fetched_nodes_;
};

}  // namespace

inline std::ostream& operator<<(std::ostream& stream,
                                const scada::NodeState& node) {
  return stream << node.node_id;
}

MATCHER_P(NodeIs, node_id, "") {
  return arg.node_id == node_id;
}

TEST(NodeFetcher, Test) {
  TestContext context;

  auto& as = context.address_space;

  InSequence s;

  // Root doesn't depend from anything.
  EXPECT_CALL(context,
              OnFetched(UnorderedElementsAre(NodeIs(scada::id::RootFolder))));

  context.node_fetcher.Fetch(scada::id::RootFolder);

  Mock::VerifyAndClearExpectations(&context);

  // TestNode1 only has a property.
  EXPECT_CALL(
      context,
      OnFetched(UnorderedElementsAre(
          NodeIs(as.kTestNode1Id),
          NodeIs(as.MakeNestedNodeId(as.kTestNode1Id, as.kTestProp1Id)))));

  context.node_fetcher.Fetch(as.kTestNode1Id);

  Mock::VerifyAndClearExpectations(&context);

  // TestNode2 depends from TestNode3.
  EXPECT_CALL(
      context,
      OnFetched(UnorderedElementsAre(
          NodeIs(as.kTestNode2Id), NodeIs(as.kTestNode3Id),
          NodeIs(as.kTestNode5Id),
          NodeIs(as.MakeNestedNodeId(as.kTestNode2Id, as.kTestProp1Id)),
          NodeIs(as.MakeNestedNodeId(as.kTestNode2Id, as.kTestProp2Id)))));

  context.node_fetcher.Fetch(as.kTestNode2Id);

  Mock::VerifyAndClearExpectations(&context);

  EXPECT_CALL(context,
              OnFetched(UnorderedElementsAre(NodeIs(as.kTestNode4Id))));

  context.node_fetcher.Fetch(as.kTestNode4Id);

  Mock::VerifyAndClearExpectations(&context);
}

TEST(NodeFetcher, UnknownNode) {
  // TODO: Read failure
  // TODO: Browse failure
}
