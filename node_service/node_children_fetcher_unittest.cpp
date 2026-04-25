#include "node_children_fetcher.h"

#include "base/test/awaitable_test.h"
#include "scada/attribute_service_mock.h"
#include "scada/coroutine_services.h"
#include "scada/node_class.h"
#include "scada/standard_node_ids.h"
//#include "scada/test/test_address_space.h"
#include "scada/view_service_mock.h"
#include "model/node_id_util.h"

#include "base/debug_util.h"

#include <map>

using namespace testing;

namespace {

struct TestContext {
  TestContext()
      : fetcher{NodeChildrenFetcher::Create(NodeChildrenFetcherContext{
            .executor_ = MakeTestAnyExecutor(executor),
            .service_context_ = {},
            .view_service_ = view_service_adapter,
            .reference_validator_ =
                [this](const scada::NodeId& node_id,
                       scada::BrowseResult&& result) {
                  validated_results.emplace(node_id, std::move(result));
                }})} {}

  void DrainExecutor() { Drain(executor); }

  std::shared_ptr<TestExecutor> executor = std::make_shared<TestExecutor>();
  StrictMock<scada::MockViewService> view_service;
  scada::CallbackToCoroutineViewServiceAdapter view_service_adapter{
      MakeTestAnyExecutor(executor), view_service};
  std::map<scada::NodeId, scada::BrowseResult> validated_results;
  std::shared_ptr<NodeChildrenFetcher> fetcher;
};

}  // namespace

MATCHER_P(NodeIs, node_id, "") {
  return arg.id == node_id;
}

TEST(NodeChildrenFetcher, Test) {
  /* TestContext context;

  auto& as = context.address_space;

  InSequence s;

  // Root doesn't depend from anything.
  EXPECT_CALL(context,
              OnFetched(scada::id::RootFolder, {scada::id::Organizes}));

  context.node_children_fetcher.FetchChildren(scada::id::RootFolder);

  // TestNode1 only has a property.
  EXPECT_CALL(
      context,
      OnFetched(UnorderedElementsAre(
          NodeIs(as.kTestNode1Id),
          NodeIs(as.MakeNestedNodeId(as.kTestNode1Id, as.kTestProp1Id)))));

  // TestNode2 depends from TestNode3.
  EXPECT_CALL(
      context,
      OnFetched(UnorderedElementsAre(
          NodeIs(as.kTestNode2Id), NodeIs(as.kTestNode3Id),
          NodeIs(as.MakeNestedNodeId(as.kTestNode2Id, as.kTestProp1Id)),
          NodeIs(as.MakeNestedNodeId(as.kTestNode2Id, as.kTestProp2Id)))));

  EXPECT_CALL(context,
              OnFetched(UnorderedElementsAre(NodeIs(as.kTestNode4Id))));

  context.node_children_fetcher.FetchChildren(scada::id::RootFolder);*/
}

TEST(NodeChildrenFetcher, Error) {
  // TODO: Read failure
  // TODO: Browse failure
}

TEST(NodeChildrenFetcher, CompletesDelayedBrowseThroughCoroutineContinuation) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  const scada::NodeId child_id{1, 101};
  scada::BrowseCallback browse_callback;

  EXPECT_CALL(context.view_service, Browse(_, SizeIs(2), _))
      .WillOnce(SaveArg<2>(&browse_callback));

  context.fetcher->Fetch(node_id);
  context.DrainExecutor();
  EXPECT_EQ(context.fetcher->GetPendingNodeCount(), 1u);
  ASSERT_TRUE(browse_callback);

  browse_callback(scada::StatusCode::Good,
                  {scada::BrowseResult{
                       .references = {{scada::id::Organizes, true, child_id}}},
                   scada::BrowseResult{}});
  context.DrainExecutor();

  EXPECT_EQ(context.fetcher->GetPendingNodeCount(), 0u);
  ASSERT_TRUE(context.validated_results.contains(node_id));
  EXPECT_THAT(context.validated_results[node_id].references,
              ElementsAre(scada::ReferenceDescription{
                  scada::id::Organizes, true, child_id}));
}

TEST(NodeChildrenFetcher, MergesMultipleBrowseResultsForNode) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  const scada::NodeId child_id{1, 101};
  const scada::NodeId subtype_id{1, 102};

  EXPECT_CALL(context.view_service, Browse(_, SizeIs(2), _))
      .WillOnce([child_id, subtype_id](
                    const scada::ServiceContext&,
                    const std::vector<scada::BrowseDescription>&,
                    const scada::BrowseCallback& callback) {
        callback(scada::StatusCode::Good,
                 {scada::BrowseResult{
                      .references = {{scada::id::Organizes, true, child_id}}},
                  scada::BrowseResult{
                      .references = {{scada::id::HasSubtype, true,
                                      subtype_id}}}});
      });

  context.fetcher->Fetch(node_id);
  context.DrainExecutor();

  ASSERT_TRUE(context.validated_results.contains(node_id));
  EXPECT_THAT(context.validated_results[node_id].references,
              UnorderedElementsAre(
                  scada::ReferenceDescription{scada::id::Organizes, true,
                                              child_id},
                  scada::ReferenceDescription{scada::id::HasSubtype, true,
                                              subtype_id}));
}

TEST(NodeChildrenFetcher, CancelRemovesQueuedNodeBeforeRequestStarts) {
  TestContext context;
  const scada::NodeId first_id{1, 100};
  const scada::NodeId second_id{1, 101};
  scada::BrowseCallback browse_callback;

  EXPECT_CALL(context.view_service, Browse(_, SizeIs(2), _))
      .WillOnce(SaveArg<2>(&browse_callback));

  context.fetcher->Fetch(first_id);
  context.DrainExecutor();
  context.fetcher->Fetch(second_id);
  context.fetcher->Cancel(second_id);

  browse_callback(scada::StatusCode::Good,
                  {scada::BrowseResult{}, scada::BrowseResult{}});
  context.DrainExecutor();

  EXPECT_TRUE(context.validated_results.contains(first_id));
  EXPECT_FALSE(context.validated_results.contains(second_id));
}
