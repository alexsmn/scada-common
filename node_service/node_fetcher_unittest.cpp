#include "node_fetcher_impl.h"

#include "address_space/test/test_address_space.h"
#include "address_space/test/test_matchers.h"
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

class TestNodeValidator {
 public:
  const scada::NodeState* GetNodeState(const scada::NodeId& node_id) const {
    auto i = fetched_nodes_.find(node_id);
    return i != fetched_nodes_.end() ? &i->second : nullptr;
  }

  bool IsNodeValid(const scada::NodeId& node_id) const {
    return fetched_nodes_.find(node_id) != fetched_nodes_.end();
  }

  void OnFetchCompleted(std::vector<scada::NodeState>&& nodes,
                        NodeFetchStatuses&& errors) {
    for (auto& node_state : nodes)
      fetched_nodes_.try_emplace(node_state.node_id, std::move(node_state));
    for (auto& [node_id, status] : errors)
      fetched_nodes_.try_emplace(node_id, scada::NodeState{});
  }

 private:
  std::map<scada::NodeId, scada::NodeState> fetched_nodes_;
};

class NodeFetcherTest : public Test {
 public:
  NodeFetcherTest();

 protected:
  void ValidateFetchedNode();

  StrictMock<MockFunction<void(std::vector<scada::NodeState>&& nodes,
                               NodeFetchStatuses&& errors)>>
      fetch_completed_handler_;

  TestNodeValidator node_validator_impl_;
  StrictMock<MockFunction<bool(const scada::NodeId& node_id)>> node_validator_;

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  TestAddressSpace server_address_space_;

  const std::shared_ptr<NodeFetcherImpl> node_fetcher_{
      NodeFetcherImpl::Create(NodeFetcherImplContext{
          executor_,
          server_address_space_,
          server_address_space_,
          fetch_completed_handler_.AsStdFunction(),
          node_validator_.AsStdFunction(),
      })};

  const scada::NodeId node_id = server_address_space_.kTestNode2Id;
  const scada::NodeId property_id =
      MakeNestedNodeId(node_id, server_address_space_.kTestProp1BrowseName);
};

}  // namespace

NodeFetcherTest::NodeFetcherTest() {
  ON_CALL(fetch_completed_handler_, Call(_, _))
      .WillByDefault(
          Invoke(&node_validator_impl_, &TestNodeValidator::OnFetchCompleted));
  ON_CALL(node_validator_, Call(_))
      .WillByDefault(
          Invoke(&node_validator_impl_, &TestNodeValidator::IsNodeValid));
}

void NodeFetcherTest::ValidateFetchedNode() {
  {
    auto* node_state = node_validator_impl_.GetNodeState(node_id);
    ASSERT_TRUE(node_state);
    EXPECT_EQ(node_state->parent_id, scada::id::RootFolder);
    EXPECT_EQ(node_state->reference_type_id, scada::id::Organizes);
    EXPECT_THAT(node_state->references,
                UnorderedElementsAre(scada::ReferenceDescription{
                    server_address_space_.kTestReferenceTypeId, true,
                    server_address_space_.kTestNode3Id}));
  }

  {
    auto* node_state = node_validator_impl_.GetNodeState(property_id);
    ASSERT_TRUE(node_state);
    EXPECT_EQ(node_state->parent_id, node_id);
    EXPECT_EQ(node_state->reference_type_id, scada::id::HasProperty);
    EXPECT_EQ(node_state->attributes.value, "TestNode2.TestProp1.Value");
  }
}

TEST_F(NodeFetcherTest, Fetch) {
  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space_, Browse(_, _)).Times(AnyNumber());
  EXPECT_CALL(node_validator_, Call(_)).Times(AnyNumber());
  EXPECT_CALL(fetch_completed_handler_, Call(_, IsEmpty())).Times(AnyNumber());
  EXPECT_CALL(fetch_completed_handler_, Call(Contains(NodeIs(node_id)), _));

  node_fetcher_->Fetch(node_id);

  ValidateFetchedNode();
}

TEST_F(NodeFetcherTest, Fetch_Force) {
  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space_, Browse(_, _)).Times(AnyNumber());
  EXPECT_CALL(node_validator_, Call(_)).Times(AnyNumber());
  EXPECT_CALL(fetch_completed_handler_, Call(_, IsEmpty())).Times(AnyNumber());
  EXPECT_CALL(fetch_completed_handler_, Call(Contains(NodeIs(node_id)), _));

  node_fetcher_->Fetch(node_id, true);

  ValidateFetchedNode();
}

TEST(NodeFetcher, UnknownNode) {
  // TODO: Read failure
  // TODO: Browse failure
}
