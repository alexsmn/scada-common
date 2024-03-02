#include "node_fetcher_impl.h"

#include "address_space/test/test_address_space.h"
#include "address_space/test/test_matchers.h"
#include "base/test/test_executor.h"
#include "model/node_id_util.h"
#include "scada/attribute_service_mock.h"
#include "scada/node_class.h"
#include "scada/service_context.h"
#include "scada/standard_node_ids.h"
#include "scada/view_service_mock.h"

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

  void OnFetchCompleted(FetchCompletedResult&& result) {
    for (auto& node_state : result.nodes)
      fetched_nodes_.try_emplace(node_state.node_id, std::move(node_state));
    for (auto& [node_id, status] : result.errors)
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

  StrictMock<MockFunction<void(FetchCompletedResult&& result)>>
      fetch_completed_handler_;

  TestNodeValidator node_validator_impl_;
  StrictMock<MockFunction<bool(const scada::NodeId& node_id)>> node_validator_;

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  TestAddressSpace server_address_space_;

  const std::shared_ptr<NodeFetcherImpl> node_fetcher_{
      NodeFetcherImpl::Create(NodeFetcherImplContext{
          executor_, server_address_space_, server_address_space_,
          fetch_completed_handler_.AsStdFunction(),
          node_validator_.AsStdFunction(),
          scada::ServiceContext::default_instance()})};

  const scada::NodeId node_id = server_address_space_.kTestNode2Id;
  const scada::NodeId property_id =
      MakeNestedNodeId(node_id, server_address_space_.kTestProp1BrowseName);
};

}  // namespace

NodeFetcherTest::NodeFetcherTest() {
  ON_CALL(fetch_completed_handler_, Call(_))
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
  EXPECT_CALL(node_validator_, Call(_)).Times(AnyNumber());

  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(AnyNumber());

  EXPECT_CALL(server_address_space_,
              Read(_, Pointee(Contains(NodeIs(node_id))), _));

  EXPECT_CALL(server_address_space_,
              Browse(/*context=*/_, /*inputs=*/_, /*callback=*/_))
      .Times(AnyNumber());

  EXPECT_CALL(server_address_space_,
              Browse(/*context=*/_, /*inputs=*/Contains(NodeIs(node_id)),
                     /*callback=*/_));

  EXPECT_CALL(fetch_completed_handler_, Call(FieldsAre(_, IsEmpty(), _)))
      .Times(AnyNumber());

  EXPECT_CALL(fetch_completed_handler_,
              Call(FieldsAre(Contains(NodeIs(node_id)), _,
                             Contains(std::make_pair(
                                 node_id, NodeFetchStatus::NodeOnly())))));

  node_fetcher_->Fetch(node_id, NodeFetchStatus::NodeOnly());

  ValidateFetchedNode();
}

TEST_F(NodeFetcherTest, Fetch_Refetch) {
  EXPECT_CALL(node_validator_, Call(_)).Times(AnyNumber());

  // Don't let upstream requests to resolve.
  EXPECT_CALL(server_address_space_, Read(_, _, _)).WillOnce([] {});
  EXPECT_CALL(server_address_space_, Browse(_, _, _)).WillOnce([] {});
  EXPECT_CALL(fetch_completed_handler_, Call(_)).Times(0);

  node_fetcher_->Fetch(node_id, NodeFetchStatus::NodeOnly());

  // Non-forced refetch doesn't trigger more upstream requests.

  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(0);
  EXPECT_CALL(server_address_space_, Browse(_, _, _)).Times(0);

  node_fetcher_->Fetch(node_id, NodeFetchStatus::NodeOnly());

  // Forced refetch triggers another set of requests.

  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space_,
              Read(_, Pointee(Contains(NodeIs(node_id))), _));
  EXPECT_CALL(server_address_space_, Browse(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space_, Browse(_, Contains(NodeIs(node_id)), _));

  EXPECT_CALL(fetch_completed_handler_, Call(FieldsAre(_, IsEmpty(), _)))
      .Times(AnyNumber());
  EXPECT_CALL(fetch_completed_handler_,
              Call(FieldsAre(Contains(NodeIs(node_id)), _,
                             Contains(std::make_pair(
                                 node_id, NodeFetchStatus::NodeOnly())))));

  node_fetcher_->Fetch(node_id, NodeFetchStatus::NodeOnly(), true);

  ValidateFetchedNode();
}

TEST_F(NodeFetcherTest, Fetch_NonHierarchicalInverseReferences) {
  EXPECT_CALL(node_validator_, Call(_)).Times(AnyNumber());

  auto fetch_status =
      NodeFetchStatus::NodeOnly().with_non_hierarchical_inverse_references();

  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space_,
              Read(_, Pointee(Contains(NodeIs(node_id))), _));
  EXPECT_CALL(server_address_space_, Browse(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space_, Browse(_, Contains(NodeIs(node_id)), _));

  EXPECT_CALL(fetch_completed_handler_, Call(FieldsAre(_, IsEmpty(), _)))
      .Times(AnyNumber());

  EXPECT_CALL(fetch_completed_handler_,
              Call(FieldsAre(Contains(NodeIs(node_id)), _,
                             Contains(std::make_pair(node_id, fetch_status)))));

  node_fetcher_->Fetch(node_id, fetch_status);

  ValidateFetchedNode();
}

TEST_F(NodeFetcherTest, UnknownNode) {
  // TODO: Read failure
  // TODO: Browse failure
}

TEST_F(NodeFetcherTest, Cancel) {
  // 1. Node fetch A is started.
  // 2. Network request A is started and takes a long time.
  // 2a. Browse request A finishes and is its references are stored in
  // |NodeFetcher|. Read request is still pending, so fetch A does not finish.
  // 3. Node fetch A canceled.
  // 4. Node fetch B is started.
  // 5. Network request B is started and takes a long time.
  // 6. Network request A finishes and is dropped.
  // 7. Network request B finished and is processed as a response for node fetch

  scada::ReadCallback read_callback1;
  scada::BrowseCallback browse_callback1;

  EXPECT_CALL(server_address_space_, Read(_, _, _))
      .WillOnce(SaveArg<2>(&read_callback1));

  EXPECT_CALL(server_address_space_, Browse(_, _, _))
      .WillOnce(SaveArg<2>(&browse_callback1));

  node_fetcher_->Fetch(node_id, NodeFetchStatus::NodeOnly());

  node_fetcher_->Cancel(node_id);

  // Second fetch.

  scada::ReadCallback read_callback2;
  scada::BrowseCallback browse_callback2;

  EXPECT_CALL(server_address_space_, Read(_, _, _))
      .WillOnce(SaveArg<2>(&read_callback2));

  EXPECT_CALL(server_address_space_, Browse(_, _, _))
      .WillOnce(SaveArg<2>(&browse_callback2));

  node_fetcher_->Fetch(node_id, NodeFetchStatus::NodeOnly());

  // First network request finishes.

  read_callback1(scada::StatusCode::Bad, {});

  // Second network request finishes.

  read_callback2(scada::StatusCode::Bad, {});
}
