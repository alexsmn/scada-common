#include "common/node_service.h"

#include "base/logger.h"
#include "common/address_space_node_service.h"
#include "common/node_id_util.h"
#include "core/configuration_impl.h"
#include "core/object.h"

#include <gmock/gmock.h>

TEST(NodeService, ReferencedNodeDeletionAndRestoration) {
  auto logger = std::make_shared<NullLogger>();
  ConfigurationImpl configuration{logger};

  const scada::NodeId kNodeId{1, 1};
  const scada::LocalizedText kDisplayName{L"DisplayName"};

  // Create node.
  {
    auto node = std::make_unique<scada::GenericObject>();
    node->set_id(kNodeId);
    node->SetDisplayName(kDisplayName);
    configuration.AddStaticNode(std::move(node));
  }

  NodeFetchStatusChecker node_fetch_status_checker =
      [&](const scada::NodeId& node_id) {
        NodeFetchStatus status{};
        status.node_fetched = configuration.GetNode(node_id) != nullptr;
        return status;
      };

  NodeFetchHandler node_fetch_handler =
      [](const scada::NodeId& node_id,
         const NodeFetchStatus& requested_status) {};

  AddressSpaceNodeService node_service{
      {logger, node_fetch_status_checker, node_fetch_handler, configuration}};

  auto node = node_service.GetNode(kNodeId);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node.id(), kNodeId);
  EXPECT_EQ(kDisplayName, node.display_name());

  // Delete node.
  configuration.RemoveNode(kNodeId);

  EXPECT_EQ(kNodeId, node.id());
  EXPECT_EQ(scada::ToLocalizedText(NodeIdToScadaString(kNodeId)),
            node.display_name());

  // Restore node.
  {
    auto node = std::make_unique<scada::GenericObject>(kNodeId, kDisplayName);
    configuration.AddStaticNode(std::move(node));
    node_service.OnNodeFetchStatusChanged(kNodeId, NodeFetchStatus::NodeOnly());
  }

  EXPECT_EQ(node.id(), kNodeId);
  EXPECT_EQ(kDisplayName, node.display_name());
}
