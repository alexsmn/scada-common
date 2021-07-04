#include "node_fetcher_impl.h"

#include "address_space/test/test_address_space.h"
#include "base/logger.h"
#include "base/test/test_executor.h"
#include "core/attribute_service_mock.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/view_service_mock.h"
#include "model/node_id_util.h"

#include "base/debug_util-inl.h"

using namespace testing;

namespace {

class NodeFetcherTest : public Test {
 protected:
  StrictMock<MockFunction<void(std::vector<scada::NodeState>&& nodes,
                               NodeFetchStatuses&& errors)>>
      fetch_completed_handler_;

  StrictMock<MockFunction<bool(const scada::NodeId& node_id)>> node_validator_;

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  TestAddressSpace server_address_space_;

  const std::shared_ptr<NodeFetcherImpl> node_fetcher_{
      NodeFetcherImpl::Create(NodeFetcherImplContext{
          executor_,
          server_address_space_,
          server_address_space_,
          fetch_completed_handler_.AsStdFunction(),
          node_validator_.AsStdFunction(),
      })};
};

}  // namespace

MATCHER_P(NodeIs, node_id, "") {
  return arg.node_id == node_id;
}

TEST_F(NodeFetcherTest, Fetch) {
  const auto node_id = server_address_space_.kTestNode1Id;

  EXPECT_CALL(server_address_space_, Read(_, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space_, Browse(_, _)).Times(AnyNumber());
  EXPECT_CALL(node_validator_, Call(_)).Times(AnyNumber());
  EXPECT_CALL(fetch_completed_handler_, Call(_, IsEmpty())).Times(AnyNumber());

  node_fetcher_->Fetch(node_id);
}

TEST(NodeFetcher, UnknownNode) {
  // TODO: Read failure
  // TODO: Browse failure
}
