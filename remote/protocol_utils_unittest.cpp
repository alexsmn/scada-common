#include <gmock/gmock.h>

#include "remote/protocol_utils.h"

TEST(ProtocolUtils, NodeId) {
  auto node_id = scada::NodeId::FromString("HISTORICAL_DB.4!PendingTaskCount");
  protocol::NodeId proto_node_id;
  ToProto(node_id, proto_node_id);
  auto restored_node_id = FromProto(proto_node_id);
  EXPECT_EQ(node_id, restored_node_id);
}
