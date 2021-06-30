#include "node_service/v1/node_fetch_status_tracker.h"

#include <gmock/gmock.h>

#include "address_space/address_space_impl.h"
#include "address_space/object.h"
#include "address_space/standard_address_space.h"
#include "base/logger.h"

using namespace testing;

namespace v1 {

class NodeFetchStatusTrackerTest : public Test {
 public:
  ~NodeFetchStatusTrackerTest();

 protected:
  StrictMock<
      MockFunction<void(base::span<const NodeFetchStatusChangedItem> items)>>
      node_fetch_status_changed_handler_;

  StrictMock<MockFunction<bool(const scada::NodeId& node_id)>> node_validator_;

  AddressSpaceImpl address_space_{std::make_shared<NullLogger>()};
  StandardAddressSpace standard_address_space_{address_space_};

  NodeFetchStatusTracker node_fetch_status_tracker_{
      {node_fetch_status_changed_handler_.AsStdFunction(),
       node_validator_.AsStdFunction(), address_space_}};

  inline static const scada::Status kGoodStatus{scada::StatusCode::Good};
};

NodeFetchStatusTrackerTest::~NodeFetchStatusTrackerTest() {
  address_space_.Clear();
}

TEST_F(NodeFetchStatusTrackerTest, GetStatus_UnknownNode) {
  const scada::NodeId node_id{1, 1};

  EXPECT_EQ(node_fetch_status_tracker_.GetStatus(node_id),
            std::make_pair(scada::Status{scada::StatusCode::Good},
                           NodeFetchStatus{}));
}

TEST_F(NodeFetchStatusTrackerTest, OnNodesFetched_GoodStatus) {
  const scada::NodeId node_id = scada::id::RootFolder;

  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(UnorderedElementsAre(NodeFetchStatusChangedItem{
                  node_id, kGoodStatus, NodeFetchStatus::NodeOnly()})));

  node_fetch_status_tracker_.OnNodesFetched({{node_id, kGoodStatus}});

  EXPECT_EQ(node_fetch_status_tracker_.GetStatus(node_id),
            std::make_pair(kGoodStatus, NodeFetchStatus::NodeOnly()));
}

TEST_F(NodeFetchStatusTrackerTest, OnNodesFetched_BadStatus) {
  const scada::NodeId node_id{1, 1};
  const scada::Status bad_status{scada::StatusCode::Bad_WrongNodeId};

  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(UnorderedElementsAre(NodeFetchStatusChangedItem{
                  node_id, bad_status, NodeFetchStatus::NodeOnly()})));

  node_fetch_status_tracker_.OnNodesFetched({{node_id, bad_status}});

  EXPECT_EQ(node_fetch_status_tracker_.GetStatus(node_id),
            std::make_pair(bad_status, NodeFetchStatus::NodeOnly()));
}

TEST_F(NodeFetchStatusTrackerTest,
       OnChildrenFetched_ParentReady_ChildrenReady) {
  const scada::NodeId node_id = scada::id::RootFolder;

  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(UnorderedElementsAre(NodeFetchStatusChangedItem{
                  node_id, kGoodStatus, NodeFetchStatus::NodeAndChildren()})));

  node_fetch_status_tracker_.OnChildrenFetched(
      node_id, {scada::ReferenceDescription{scada::id::Organizes, true,
                                            scada::id::ObjectsFolder},
                scada::ReferenceDescription{scada::id::Organizes, true,
                                            scada::id::TypesFolder}});

  EXPECT_EQ(node_fetch_status_tracker_.GetStatus(node_id),
            std::make_pair(kGoodStatus, NodeFetchStatus::NodeAndChildren()));
}

TEST_F(NodeFetchStatusTrackerTest,
       OnChildrenFetched_ParentReady_ChildrenDelayed) {
  const scada::NodeId parent_id = scada::id::RootFolder;
  const scada::NodeId child_id{1, 1};

  EXPECT_CALL(node_validator_, Call(child_id));

  node_fetch_status_tracker_.OnChildrenFetched(
      parent_id,
      {scada::ReferenceDescription{scada::id::Organizes, true,
                                   scada::id::ObjectsFolder},
       scada::ReferenceDescription{scada::id::Organizes, true,
                                   scada::id::TypesFolder},
       scada::ReferenceDescription{scada::id::Organizes, true, child_id}});

  // Resolve children.

  EXPECT_CALL(
      node_fetch_status_changed_handler_,
      Call(UnorderedElementsAre(
          NodeFetchStatusChangedItem{child_id, kGoodStatus,
                                     NodeFetchStatus::NodeOnly()},
          NodeFetchStatusChangedItem{parent_id, kGoodStatus,
                                     NodeFetchStatus::NodeAndChildren()})));

  address_space_.AddStaticNode<scada::GenericObject>(child_id, "ChildName",
                                                     L"ChildDisplayName");

  node_fetch_status_tracker_.OnNodesFetched(
      {std::make_pair(child_id, kGoodStatus)});

  EXPECT_EQ(node_fetch_status_tracker_.GetStatus(child_id),
            std::make_pair(kGoodStatus, NodeFetchStatus::NodeOnly()));
  EXPECT_EQ(node_fetch_status_tracker_.GetStatus(parent_id),
            std::make_pair(kGoodStatus, NodeFetchStatus::NodeAndChildren()));
}

TEST_F(NodeFetchStatusTrackerTest,
       OnChildrenFetched_ParentDelayed_ChildrenReady) {
  const scada::NodeId parent_id{1, 1};

  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(UnorderedElementsAre(NodeFetchStatusChangedItem{
                  parent_id, kGoodStatus, NodeFetchStatus::ChildrenOnly()})));

  node_fetch_status_tracker_.OnChildrenFetched(
      parent_id, {scada::ReferenceDescription{scada::id::Organizes, true,
                                              scada::id::ObjectsFolder},
                  scada::ReferenceDescription{scada::id::Organizes, true,
                                              scada::id::TypesFolder}});

  // Resolve parent.

  EXPECT_EQ(node_fetch_status_tracker_.GetStatus(parent_id),
            std::make_pair(kGoodStatus, NodeFetchStatus::ChildrenOnly()));

  address_space_.AddStaticNode<scada::GenericObject>(parent_id, "ParentName",
                                                     L"ParentDisplayName");

  EXPECT_CALL(
      node_fetch_status_changed_handler_,
      Call(UnorderedElementsAre(NodeFetchStatusChangedItem{
          parent_id, kGoodStatus, NodeFetchStatus::NodeAndChildren()})));

  node_fetch_status_tracker_.OnNodesFetched(
      {std::make_pair(parent_id, kGoodStatus)});

  EXPECT_EQ(node_fetch_status_tracker_.GetStatus(parent_id),
            std::make_pair(kGoodStatus, NodeFetchStatus::NodeAndChildren()));
}

}  // namespace v1
