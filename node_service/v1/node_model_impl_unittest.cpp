#include "node_service/v1/node_model_impl.h"

#include "address_space/address_space_impl.h"
#include "address_space/object.h"
#include "scada/attribute_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item.h"
#include "scada/monitored_item_service_mock.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

namespace v1 {

using namespace testing;

class MockAddressSpaceNodeModelDelegate : public NodeModelDelegate {
 public:
  MOCK_METHOD(NodeRef, GetRemoteNode, (const scada::Node* node), (override));
  MOCK_METHOD(void,
              OnNodeModelDeleted,
              (const scada::NodeId& node_id),
              (override));
  MOCK_METHOD(void,
              OnNodeModelFetchRequested,
              (const scada::NodeId& node_id,
               const NodeFetchStatus& requested_status),
              (override));
};

TEST(NodeModelImpl, Fetch) {
  const scada::NodeId kNodeId = scada::id::RootFolder;

  AddressSpaceImpl address_space;
  MockAddressSpaceNodeModelDelegate delegate;
  NiceMock<scada::MockAttributeService> attribute_service;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service;
  NiceMock<scada::MockMethodService> method_service;
  NodeModelImpl node{NodeModelImplContext{
      delegate,
      kNodeId,
      attribute_service,
      monitored_item_service,
      method_service,
  }};
  EXPECT_CALL(delegate,
              OnNodeModelFetchRequested(kNodeId, NodeFetchStatus::NodeOnly()));

  bool fetched = false;
  node.Fetch(NodeFetchStatus::NodeOnly(), [&] { fetched = true; });

  EXPECT_CALL(delegate, OnNodeModelFetchRequested(
                            kNodeId, NodeFetchStatus::NodeAndChildren()));

  bool children_fetched = false;
  node.Fetch(NodeFetchStatus::NodeAndChildren(),
             [&] { children_fetched = true; });

  EXPECT_TRUE(node.GetFetchStatus().empty());
  EXPECT_FALSE(fetched);
  EXPECT_FALSE(children_fetched);

  auto& root_folder =
      address_space.AddStaticNode(std::make_unique<scada::GenericObject>(
          scada::id::RootFolder, "RootFolder", scada::LocalizedText{}));
  node.SetFetchStatus(&root_folder, scada::StatusCode::Good,
                      NodeFetchStatus::NodeOnly());

  EXPECT_TRUE(fetched);
  EXPECT_FALSE(children_fetched);

  node.SetFetchStatus(&root_folder, scada::StatusCode::Good,
                      NodeFetchStatus::NodeAndChildren());
  EXPECT_TRUE(fetched);
  EXPECT_TRUE(children_fetched);
}

}  // namespace v1
