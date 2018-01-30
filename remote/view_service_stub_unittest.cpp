#include "view_service_stub.h"

#include <gmock/gmock.h>

#include "common/node_state.h"
#include "core/test/test_address_space.h"
#include "protocol.h"
#include "protocol_utils.h"

using namespace testing;

namespace scada {

bool operator==(const NodeAttributes& a, const NodeAttributes& b) {
  return std::tie(a.browse_name, a.data_type, a.value) ==
         std::tie(b.browse_name, b.data_type, b.value);
}

bool operator==(const NodeState& a, const NodeState& b) {
  return a.node_id == b.node_id && a.node_class == b.node_class &&
         a.type_definition_id == b.type_definition_id &&
         a.parent_id == b.parent_id &&
         a.reference_type_id == b.reference_type_id &&
         a.attributes == b.attributes && a.properties == b.properties;
}

}  // namespace scada

/*TEST(ViewServiceStub, BrowseTree) {
  MockMessageSender message_sender;
  test::TestAddressSpace address_space;
  const auto logger = std::make_shared<NullLogger>();
  ViewServiceStub view_service_stub{message_sender, address_space, logger};

  const unsigned request_id = 353152;

  // Expected response.
  EXPECT_CALL(message_sender, Send(_))
      .WillOnce(Invoke([&](const protocol::Message& message) {
        ASSERT_TRUE(message.responses_size() == 1);
        auto& response = message.responses(0);
        EXPECT_EQ(scada::StatusCode::Good, FromProto(response.status()).code());
        ASSERT_TRUE(response.has_browse_tree());
        auto& browse_tree = response.browse_tree();
        auto& node = browse_tree.node();
        EXPECT_EQ(address_space.kTestNode1Id, FromProto(node.node_id()));
      }));

  // Request.
  {
    protocol::Request request;
    request.set_request_id(request_id);
    auto& browse_tree = *request.mutable_browse_tree();
    ToProto(address_space.kTestNode1Id, *browse_tree.mutable_node_id());
    view_service_stub.OnRequestReceived(request);
  }
}*/
