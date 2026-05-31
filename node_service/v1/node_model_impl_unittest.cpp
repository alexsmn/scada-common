#include "node_service/v1/node_model_impl.h"

#include "address_space/address_space_impl.h"
#include "address_space/object.h"
#include "scada/monitored_item.h"

#include <gmock/gmock.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include "base/debug_util.h"

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
  NodeModelImpl node{NodeModelImplContext{
      .delegate_ = delegate,
      .node_id_ = kNodeId,
      .scada_node_ = {},
  }};
  EXPECT_CALL(delegate,
              OnNodeModelFetchRequested(kNodeId, NodeFetchStatus::NodeOnly()));

  boost::asio::io_context io_context;
  bool fetched = false;
  auto fetched_result = boost::asio::co_spawn(
      io_context,
      [&node, &fetched]() -> Awaitable<void> {
        co_await node.Fetch(NodeFetchStatus::NodeOnly());
        fetched = true;
      },
      boost::asio::use_future);
  io_context.poll();

  EXPECT_CALL(delegate, OnNodeModelFetchRequested(
                            kNodeId, NodeFetchStatus::NodeAndChildren()));

  bool children_fetched = false;
  auto children_fetched_result = boost::asio::co_spawn(
      io_context,
      [&node, &children_fetched]() -> Awaitable<void> {
        co_await node.Fetch(NodeFetchStatus::NodeAndChildren());
        children_fetched = true;
      },
      boost::asio::use_future);
  io_context.poll();

  EXPECT_TRUE(node.GetFetchStatus().empty());
  EXPECT_FALSE(fetched);
  EXPECT_FALSE(children_fetched);

  auto& root_folder =
      address_space.AddStaticNode(std::make_unique<scada::GenericObject>(
          scada::id::RootFolder, "RootFolder", scada::LocalizedText{}));
  node.SetFetchStatus(&root_folder, scada::StatusCode::Good,
                      NodeFetchStatus::NodeOnly());
  io_context.poll();

  EXPECT_TRUE(fetched);
  EXPECT_FALSE(children_fetched);

  node.SetFetchStatus(&root_folder, scada::StatusCode::Good,
                      NodeFetchStatus::NodeAndChildren());
  io_context.restart();
  io_context.run();
  EXPECT_TRUE(fetched);
  EXPECT_TRUE(children_fetched);

  fetched_result.get();
  children_fetched_result.get();
}

}  // namespace v1
