#include "node_service/node_service_factory.h"

#include "address_space/test/test_address_space.h"
#include "base/test/test_executor.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/coroutine_services.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/standard_node_ids.h"

#include <boost/signals2/signal.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using testing::NiceMock;

class TestCoroutineSessionService final : public scada::CoroutineSessionService {
 public:
  Awaitable<void> Connect(scada::SessionConnectParams params) override {
    co_return;
  }

  Awaitable<void> Reconnect() override { co_return; }

  Awaitable<void> Disconnect() override { co_return; }

  bool IsConnected(base::TimeDelta* ping_delay = nullptr) const override {
    ++is_connected_count;
    return connected;
  }

  scada::NodeId GetUserId() const override { return {}; }

  bool HasPrivilege(scada::Privilege privilege) const override { return true; }

  std::string GetHostName() const override { return {}; }

  bool IsScada() const override { return true; }

  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override {
    ++subscribe_count;
    return session_state_changed.connect(callback);
  }

  scada::SessionDebugger* GetSessionDebugger() override { return nullptr; }

  bool connected = true;
  mutable int is_connected_count = 0;
  int subscribe_count = 0;
  boost::signals2::signal<void(bool, const scada::Status&)>
      session_state_changed;
};

void DrainExecutor(const std::shared_ptr<TestExecutor>& executor) {
  for (size_t i = 0; i < 100 && executor->GetTaskCount() != 0; ++i)
    executor->Poll();
}

std::shared_ptr<NodeService> CreateCoroutineFactoryNodeService(
    TestAddressSpace& address_space,
    scada::CoroutineSessionService& session_service,
    scada::MonitoredItemService& monitored_item_service,
    const std::shared_ptr<TestExecutor>& executor,
    bool use_v2) {
  return CreateNodeService(
      CoroutineNodeServiceContext{
          .executor_ = MakeTestAnyExecutor(executor),
          .service_context_ = scada::ServiceContext{},
          .session_service_ = session_service,
          .attribute_service_ = address_space.attribute_service_impl,
          .view_service_ = address_space.view_service_impl,
          .monitored_item_service_ = monitored_item_service,
          .scada_client_ = {}},
      use_v2);
}

void ExpectCoroutineFactoryFetchesNode(bool use_v2) {
  const auto executor = std::make_shared<TestExecutor>();
  TestAddressSpace address_space;
  TestCoroutineSessionService session_service;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service;

  const auto node_service = CreateCoroutineFactoryNodeService(
      address_space, session_service, monitored_item_service, executor, use_v2);
  auto node = node_service->GetNode(address_space.kTestNode2Id);

  node.Fetch(NodeFetchStatus::NodeAndChildren());
  DrainExecutor(executor);

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.children_fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.node_class(), scada::NodeClass::Object);
  EXPECT_EQ(node.browse_name(), "TestNode2");
  EXPECT_EQ(node.display_name(), u"TestNode2DisplayName");
  EXPECT_EQ(node.target(address_space.kTestReferenceTypeId).node_id(),
            address_space.kTestNode3Id);
  EXPECT_EQ(session_service.subscribe_count, 1);
  EXPECT_EQ(session_service.is_connected_count, 1);
}

TEST(NodeServiceFactory, V1CoroutineContextFetchesThroughCoroutineServices) {
  ExpectCoroutineFactoryFetchesNode(/*use_v2=*/false);
}

TEST(NodeServiceFactory, V2CoroutineContextFetchesThroughCoroutineServices) {
  ExpectCoroutineFactoryFetchesNode(/*use_v2=*/true);
}

}  // namespace
