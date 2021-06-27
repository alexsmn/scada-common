#include "node_fetcher_impl.h"

#include "address_space/test/test_address_space.h"
#include "base/logger.h"
#include "base/test/test_executor.h"
#include "core/attribute_service_mock.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/view_service_mock.h"
#include "model/node_id_util.h"

#include "base/debug_util-inl.h"

using namespace testing;

namespace {

struct TestContext {
  MOCK_METHOD1(OnFetched, void(std::vector<scada::NodeState>& nodes));
  MOCK_METHOD2(OnFetchError,
               void(const scada::NodeId& node_id, const scada::Status& status));

  void ProcessFetchedNodes(const std::vector<scada::NodeState>& nodes) {
    for (auto& node : nodes)
      fetched_nodes_[node.node_id] = node;
  }

  const std::shared_ptr<TestExecutor> executor =
      std::make_shared<TestExecutor>();

  TestAddressSpace address_space;

  const std::shared_ptr<NodeFetcherImpl> node_fetcher{
      NodeFetcherImpl::Create(NodeFetcherImplContext{
          executor,
          address_space,
          address_space,
          [&](std::vector<scada::NodeState>&& nodes,
              NodeFetchStatuses&& errors) {
            OnFetched(nodes);
            ProcessFetchedNodes(nodes);
            for (auto& [node_id, status] : errors)
              OnFetchError(node_id, status);
          },
          [&](const scada::NodeId& node_id) {
            return fetched_nodes_.find(node_id) != fetched_nodes_.end();
          },
      })};

  std::map<scada::NodeId, scada::NodeState> fetched_nodes_;
};

}  // namespace

MATCHER_P(NodeIs, node_id, "") {
  return arg.node_id == node_id;
}

TEST(NodeFetcher, DISABLED_Test) {
  TestContext context;

  auto& as = context.address_space;

  InSequence s;

  // Don't expect any errors.
  EXPECT_CALL(context, OnFetchError(_, _)).Times(0);

  // Root doesn't depend from anything.
  EXPECT_CALL(context, OnFetched(Contains(NodeIs(scada::id::RootFolder))));

  context.node_fetcher->Fetch(scada::id::RootFolder);

  Mock::VerifyAndClearExpectations(&context);

  // TestNode1 only has a property.
  EXPECT_CALL(context,
              OnFetched(AllOf(Contains(NodeIs(as.kTestNode1Id)),
                              Contains(NodeIs(as.MakeNestedNodeId(
                                  as.kTestNode1Id, as.kTestProp1Id))))));

  context.node_fetcher->Fetch(as.kTestNode1Id);

  Mock::VerifyAndClearExpectations(&context);

  EXPECT_CALL(context,
              OnFetched(AllOf(Contains(NodeIs(as.kTestNode2Id)),
                              Contains(NodeIs(as.kTestNode3Id)),
                              Contains(NodeIs(as.kTestNode5Id)),
                              Contains(NodeIs(as.MakeNestedNodeId(
                                  as.kTestNode2Id, as.kTestProp1Id))),
                              Contains(NodeIs(as.MakeNestedNodeId(
                                  as.kTestNode2Id, as.kTestProp2Id))))));

  context.node_fetcher->Fetch(as.kTestNode2Id);

  Mock::VerifyAndClearExpectations(&context);

  EXPECT_CALL(context, OnFetched(Contains(NodeIs(as.kTestNode4Id))));

  context.node_fetcher->Fetch(as.kTestNode4Id);

  Mock::VerifyAndClearExpectations(&context);
}

TEST(NodeFetcher, UnknownNode) {
  // TODO: Read failure
  // TODO: Browse failure
}
