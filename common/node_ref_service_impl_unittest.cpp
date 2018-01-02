#include "node_ref_service_impl.h"

#include "base/logger.h"
#include "core/attribute_service.h"
#include "core/test/test_address_space.h"

#include <gmock/gmock.h>

using namespace test;
using namespace testing;

class MockAttributeService : public scada::AttributeService {
 public:
  MOCK_METHOD2(Read, void(const std::vector<scada::ReadValueId>& value_ids, const scada::ReadCallback& callback));
  MOCK_METHOD5(Write, void(const scada::NodeId& node_id, double value, const scada::NodeId& user_id,
                           const scada::WriteFlags& flags, const scada::StatusCallback& callback));
};

/*struct NodeRefServiceImplTestContext {
  const std::shared_ptr<Logger> logger = std::make_shared<NullLogger>();
  TestAddressSpace address_space;
  TestViewService view_service{address_space};
  MockAttributeService attribute_service;
  NodeRefServiceImpl node_service{NodeRefServiceImplContext{
      logger,
      view_service,
      attribute_service,
  }};
};*/

TEST(NodeRefServiceImpl, FetchNode) {
  const auto logger = std::make_shared<NullLogger>();
  TestAddressSpace address_space;
  MockAttributeService attribute_service;
  NodeRefServiceImpl node_service{NodeRefServiceImplContext{
      logger,
      address_space,
      attribute_service,
  }};

  std::vector<scada::ReadValueId> pending_read_ids;
  scada::ReadCallback pending_read_callback;  
  EXPECT_CALL(attribute_service, Read(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::ReadValueId>& read_ids, const scada::ReadCallback& callback) {
        pending_read_ids = read_ids;
        pending_read_callback = callback;
      }));

  auto* server_node = address_space.GetNode(scada::id::RootFolder);
  ASSERT_NE(nullptr, server_node);

  auto node = node_service.GetNode(server_node->node_id);
  EXPECT_FALSE(node.fetched());

  // Check pending node.
  {
    EXPECT_EQ(scada::QualifiedName{server_node->node_id.ToString()}, node.browse_name());
    EXPECT_EQ(scada::LocalizedText{server_node->node_id.ToString()}, node.display_name());
    EXPECT_EQ(std::nullopt, node.node_class());
    // TODO: More checks for pending node.
  }

  Mock::VerifyAndClearExpectations(&attribute_service);

  // Read succeeds.
  {
    // Respond read attributes request.
    ASSERT_TRUE(pending_read_callback);
    std::vector<scada::DataValue> read_results(pending_read_ids.size());
    for (size_t i = 0; i < pending_read_ids.size(); ++i) {
      auto& read_id = pending_read_ids[i];
      auto& result = read_results[i];
      EXPECT_EQ(server_node->node_id, read_id.node_id);
      result = address_space.Read(read_id);
    }
    pending_read_callback(scada::StatusCode::Good, std::move(read_results));

    /*EXPECT_TRUE(node.fetched());
    EXPECT_EQ(address_space.kRootNode.browse_name, node.browse_name());*/
  }

  // Read fails.
  {
    // TODO:
  }
}
