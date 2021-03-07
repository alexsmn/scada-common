#include "node_service/v1/node_model_impl.h"

#include "address_space/address_space_impl.h"
#include "address_space/object.h"
#include "base/logger.h"
#include "base/strings/utf_string_conversions.h"
#include "core/attribute_service_mock.h"
#include "core/method_service_mock.h"
#include "core/monitored_item.h"
#include "core/monitored_item_service_mock.h"

#include <gmock/gmock.h>

namespace v1 {

using namespace testing;

class MockAddressSpaceNodeModelDelegate : public NodeModelDelegate {
 public:
  MOCK_METHOD1(GetRemoteNode, NodeRef(const scada::Node* node));
  MOCK_METHOD1(OnNodeModelDeleted, void(const scada::NodeId& node_id));
  MOCK_METHOD2(OnNodeModelFetchRequested,
               void(const scada::NodeId& node_id,
                    const NodeFetchStatus& requested_status));
};

TEST(NodeModelImpl, Fetch) {
  const scada::NodeId kNodeId = scada::id::RootFolder;

  AddressSpaceImpl address_space{std::make_shared<NullLogger>()};
  MockAddressSpaceNodeModelDelegate delegate;
  NiceMock<scada::MockAttributeService> attribute_service;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service;
  NiceMock<scada::MockMethodService> method_service;
  NodeModelImpl node{AddressSpaceNodeModelContext{
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
