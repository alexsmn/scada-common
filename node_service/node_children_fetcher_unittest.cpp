#include "node_children_fetcher.h"

#include "scada/attribute_service_mock.h"
#include "scada/node_class.h"
#include "scada/standard_node_ids.h"
//#include "scada/test/test_address_space.h"
#include "scada/view_service_mock.h"
#include "model/node_id_util.h"

#include "base/debug_util-inl.h"

using namespace testing;

namespace {

struct TestContext {
  /*  MOCK_METHOD2(OnFetched,
                 void(const scada::NodeId& node_id,
                      const ReferenceMap& references));*/

  // TestAddressSpace address_space;

  /*NodeChildrenFetcher node_children_fetcher{NodeChildrenFetcherContext{
      logger,
      address_space,
      [&](const scada::NodeId& node_id, ReferenceMap references) {
        this->references[node_id] = std::move(references);
      },
  }};*/

  //  std::map<scada::NodeId, ReferenceMap> references;
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
