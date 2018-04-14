#include "address_space_node_model.h"

#include "base/logger.h"
#include "address_space/address_space_impl.h"
#include "address_space/object.h"

#include <gmock/gmock.h>

class MockAddressSpaceNodeModelDelegate : public AddressSpaceNodeModelDelegate {
 public:
  MOCK_METHOD1(GetRemoteNode, NodeRef(const scada::Node* node));
  MOCK_METHOD1(OnRemoteNodeModelDeleted, void(const scada::NodeId& node_id));
};

TEST(AddressSpaceNodeModel, Fetch) {
  AddressSpaceImpl address_space{std::make_shared<NullLogger>()};
  MockAddressSpaceNodeModelDelegate delegate;
  AddressSpaceNodeModel node{delegate, scada::id::RootFolder};

  bool fetched = false;
  node.Fetch(NodeFetchStatus::NodeOnly(), [&] { fetched = true; });
  bool children_fetched = false;
  node.Fetch(NodeFetchStatus::NodeAndChildren(), [&] { children_fetched = true; });

  EXPECT_TRUE(node.GetFetchStatus().empty());
  EXPECT_FALSE(fetched);
  EXPECT_FALSE(children_fetched);

  auto& root_folder =
      address_space.AddStaticNode(std::make_unique<scada::GenericObject>(
          scada::id::RootFolder, L"RootFolder"));
  node.OnNodeFetchStatusChanged(&root_folder, NodeFetchStatus::NodeOnly());

  EXPECT_TRUE(fetched);
  EXPECT_FALSE(children_fetched);

  node.OnNodeFetchStatusChanged(&root_folder, NodeFetchStatus::NodeAndChildren());
  EXPECT_TRUE(fetched);
  EXPECT_TRUE(children_fetched);
}
