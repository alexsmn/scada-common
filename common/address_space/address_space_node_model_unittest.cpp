#include "address_space_node_model.h"

#include "address_space/address_space_impl.h"
#include "address_space/object.h"
#include "base/logger.h"
#include "base/strings/utf_string_conversions.h"

#include <gmock/gmock.h>

class MockAddressSpaceNodeModelDelegate : public AddressSpaceNodeModelDelegate {
 public:
  MOCK_METHOD1(GetRemoteNode, NodeRef(const scada::Node* node));
  MOCK_METHOD1(OnNodeModelDeleted, void(const scada::NodeId& node_id));
  MOCK_METHOD2(OnNodeModelFetchRequested,
               void(const scada::NodeId& node_id,
                    const NodeFetchStatus& requested_status));
};

TEST(AddressSpaceNodeModel, Fetch) {
  const scada::NodeId kNodeId = scada::id::RootFolder;

  AddressSpaceImpl address_space{std::make_shared<NullLogger>()};
  MockAddressSpaceNodeModelDelegate delegate;
  AddressSpaceNodeModel node{delegate, kNodeId};

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
          scada::id::RootFolder, base::WideToUTF16(L"RootFolder")));
  node.SetFetchStatus(&root_folder, scada::StatusCode::Good,
                      NodeFetchStatus::NodeOnly());

  EXPECT_TRUE(fetched);
  EXPECT_FALSE(children_fetched);

  node.SetFetchStatus(&root_folder, scada::StatusCode::Good,
                      NodeFetchStatus::NodeAndChildren());
  EXPECT_TRUE(fetched);
  EXPECT_TRUE(children_fetched);
}
