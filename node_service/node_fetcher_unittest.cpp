#include "node_fetcher_impl.h"

#include "address_space/test/test_address_space.h"
#include "address_space/test/test_matchers.h"
#include "base/test/test_executor.h"
#include "base/executor_conversions.h"
#include "model/node_id_util.h"
#include "scada/attribute_service_mock.h"
#include "scada/coroutine_services.h"
#include "scada/node_class.h"
#include "scada/service_context.h"
#include "scada/standard_node_ids.h"
#include "scada/view_service_mock.h"

#include "base/debug_util.h"

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
  void DrainExecutor() {
    for (size_t i = 0; i < 100 && executor_->GetTaskCount() != 0; ++i)
      executor_->Poll();
  }

  void ValidateFetchedNode();

  StrictMock<MockFunction<void(FetchCompletedResult&& result)>>
      fetch_completed_handler_;

  TestNodeValidator node_validator_impl_;
  StrictMock<MockFunction<bool(const scada::NodeId& node_id)>> node_validator_;

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  TestAddressSpace server_address_space_;
  scada::CallbackToCoroutineViewServiceAdapter view_service_adapter_{
      MakeTestAnyExecutor(executor_), server_address_space_};
  scada::CallbackToCoroutineAttributeServiceAdapter attribute_service_adapter_{
      MakeTestAnyExecutor(executor_), server_address_space_};

  const std::shared_ptr<NodeFetcherImpl> node_fetcher_{
      NodeFetcherImpl::Create(NodeFetcherImplContext{
          MakeTestAnyExecutor(executor_), view_service_adapter_,
          attribute_service_adapter_,
          fetch_completed_handler_.AsStdFunction(),
          node_validator_.AsStdFunction()})};

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
  DrainExecutor();

  ValidateFetchedNode();
}

TEST_F(NodeFetcherTest, Fetch_Refetch) {
  EXPECT_CALL(node_validator_, Call(_)).Times(AnyNumber());

  // Don't let upstream requests to resolve.
  EXPECT_CALL(server_address_space_, Read(_, _, _)).WillOnce([] {});
  EXPECT_CALL(server_address_space_, Browse(_, _, _)).WillOnce([] {});
  EXPECT_CALL(fetch_completed_handler_, Call(_)).Times(0);

  node_fetcher_->Fetch(node_id, NodeFetchStatus::NodeOnly());
  DrainExecutor();

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
  DrainExecutor();

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
  DrainExecutor();

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
  DrainExecutor();

  node_fetcher_->Cancel(node_id);

  // Second fetch.

  scada::ReadCallback read_callback2;
  scada::BrowseCallback browse_callback2;

  EXPECT_CALL(server_address_space_, Read(_, _, _))
      .WillOnce(SaveArg<2>(&read_callback2));

  EXPECT_CALL(server_address_space_, Browse(_, _, _))
      .WillOnce(SaveArg<2>(&browse_callback2));

  node_fetcher_->Fetch(node_id, NodeFetchStatus::NodeOnly());
  DrainExecutor();

  // First network request finishes.

  read_callback1(scada::StatusCode::Bad, {});
  DrainExecutor();

  // Second network request finishes.

  read_callback2(scada::StatusCode::Bad, {});
  DrainExecutor();
}

// Regression test for the fetcher-queue drain recursion observed in
// the screenshot generator. Historically `OnReadResult` /
// `OnBrowseResult` called `NotifyFetchedNodes` → completion handler
// after the `processing_response_` guard had been reset; with
// in-process synchronous services, any `Fetch()` the handler
// scheduled ran the full Read/Browse → OnResult → Notify → handler
// chain on top of the current stack, overflowing it in deep trees.
//
// The fix wraps the top-level `FetchPendingNodes()` in a
// `draining_pending_queue_` guard and a drain loop. Nested `Fetch()`
// calls (from handlers) enqueue and return; the outer loop picks the
// new work up iteratively.
//
// This test walks the six TestAddressSpace nodes head-to-tail and
// verifies every node was fetched while the handler never re-entered
// on the stack.
TEST_F(NodeFetcherTest, FetchCompletedHandlerReentryIsIterative) {
  const std::vector<scada::NodeId> chain{
      server_address_space_.kTestNode1Id, server_address_space_.kTestNode2Id,
      server_address_space_.kTestNode3Id, server_address_space_.kTestNode4Id,
      server_address_space_.kTestNode5Id, server_address_space_.kTestNode6Id,
  };

  EXPECT_CALL(node_validator_, Call(_)).Times(AnyNumber());
  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space_, Browse(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(fetch_completed_handler_, Call(_)).Times(AnyNumber());

  int depth = 0;
  int max_depth = 0;

  ON_CALL(fetch_completed_handler_, Call(_))
      .WillByDefault([&](FetchCompletedResult&& result) {
        ++depth;
        if (depth > max_depth)
          max_depth = depth;

        node_validator_impl_.OnFetchCompleted(std::move(result));

        // Walk to the next link in the chain only if the node we just
        // finished is in the chain; otherwise ignore (auxiliary fetches
        // triggered by the fetcher for references/properties).
        for (size_t i = 0; i + 1 < chain.size(); ++i) {
          if (node_validator_impl_.IsNodeValid(chain[i]) &&
              !node_validator_impl_.IsNodeValid(chain[i + 1])) {
            node_fetcher_->Fetch(chain[i + 1], NodeFetchStatus::NodeOnly());
            break;
          }
        }

        --depth;
      });

  node_fetcher_->Fetch(chain.front(), NodeFetchStatus::NodeOnly());
  DrainExecutor();

  // Every chain node must have been fetched: if the completion handler
  // re-entry had been deferred through an executor, the first handler
  // call would have returned before the second Fetch got a chance to
  // run and the test would have ended after a single node.
  for (const auto& id : chain)
    EXPECT_TRUE(node_validator_impl_.IsNodeValid(id))
        << "Chain node " << NodeIdToScadaString(id) << " was not fetched";

  // With the iterative drain the handler fires once per batch and
  // always at depth 1 — never nested. If this assertion fails the
  // drain is recursing again and the screenshot generator will
  // overflow its stack.
  EXPECT_EQ(max_depth, 1)
      << "fetch_completed_handler_ re-entered on the same stack frame "
         "— the drain is no longer iterative and the screenshot "
         "generator will regress to a stack overflow. max_depth="
      << max_depth;
}
