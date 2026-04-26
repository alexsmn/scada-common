#include "common/master_data_services.h"

#include "base/awaitable_promise.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "scada/attribute_service_mock.h"
#include "scada/node_management_service_mock.h"
#include "scada/session_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <vector>

namespace {

using namespace std::chrono_literals;
using testing::_;
using testing::StrictMock;

TEST(MasterDataServicesTest, CallbackReadDispatchesThroughCoroutineAdapter) {
  auto executor = std::make_shared<TestExecutor>();
  MasterDataServices services{MakeTestAnyExecutor(executor)};
  auto attribute_service =
      std::make_shared<StrictMock<scada::MockAttributeService>>();

  DataServices data_services;
  data_services.attribute_service_ = attribute_service;
  services.SetServices(std::move(data_services));

  scada::ReadCallback pending_read;
  EXPECT_CALL(*attribute_service, Read(_, _, _))
      .WillOnce([&](const scada::ServiceContext&,
                    const std::shared_ptr<const std::vector<scada::ReadValueId>>&,
                    const scada::ReadCallback& callback) {
        pending_read = callback;
      });

  bool read_called = false;
  services.Read(
      {}, std::make_shared<const std::vector<scada::ReadValueId>>(),
      [&](scada::Status status, std::vector<scada::DataValue> results) {
        read_called = true;
        EXPECT_TRUE(status.good());
        EXPECT_EQ(results.size(), 1u);
      });

  Drain(executor);
  EXPECT_FALSE(read_called);
  ASSERT_TRUE(pending_read);

  pending_read(scada::StatusCode::Good, {scada::DataValue{}});
  Drain(executor);

  EXPECT_TRUE(read_called);
}

TEST(MasterDataServicesTest,
     CoroutineNodeManagementUsesCallbackAdapterWithDelayedCompletion) {
  auto executor = std::make_shared<TestExecutor>();
  MasterDataServices services{MakeTestAnyExecutor(executor)};
  auto node_management_service =
      std::make_shared<StrictMock<scada::MockNodeManagementService>>();

  DataServices data_services;
  data_services.node_management_service_ = node_management_service;
  services.SetServices(std::move(data_services));

  scada::AddNodesCallback pending_add_nodes;
  EXPECT_CALL(*node_management_service, AddNodes(_, _))
      .WillOnce([&](const std::vector<scada::AddNodesItem>&,
                    const scada::AddNodesCallback& callback) {
        pending_add_nodes = callback;
      });

  auto result = ToPromise(
      MakeTestAnyExecutor(executor),
      static_cast<scada::CoroutineNodeManagementService&>(services).AddNodes(
          {scada::AddNodesItem{.requested_id = scada::NodeId{1}}}));
  Drain(executor);

  EXPECT_EQ(result.wait_for(0ms), promise_wait_status::timeout);
  ASSERT_TRUE(pending_add_nodes);

  pending_add_nodes(scada::StatusCode::Good,
                    {scada::AddNodesResult{.added_node_id = scada::NodeId{1}}});

  auto [status, results] = WaitPromise(executor, std::move(result));
  EXPECT_TRUE(status.good());
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].added_node_id, scada::NodeId{1});
}

TEST(MasterDataServicesTest, SessionConnectPromiseUsesCoroutineAdapter) {
  auto executor = std::make_shared<TestExecutor>();
  MasterDataServices services{MakeTestAnyExecutor(executor)};
  auto session_service =
      std::make_shared<StrictMock<scada::MockSessionService>>();

  DataServices data_services;
  data_services.session_service_ = session_service;
  EXPECT_CALL(*session_service, SubscribeSessionStateChanged(_));
  services.SetServices(std::move(data_services));

  promise<void> pending_connect;
  EXPECT_CALL(*session_service, Connect(_))
      .WillOnce([&](const scada::SessionConnectParams&) {
        return pending_connect;
      });

  auto result = services.Connect({});
  Drain(executor);

  EXPECT_EQ(result.wait_for(0ms), promise_wait_status::timeout);

  pending_connect.resolve();
  WaitPromise(executor, std::move(result));
}

TEST(MasterDataServicesTest, CoroutineSessionFacadeUsesPromiseAdapter) {
  auto executor = std::make_shared<TestExecutor>();
  MasterDataServices services{MakeTestAnyExecutor(executor)};
  auto session_service =
      std::make_shared<StrictMock<scada::MockSessionService>>();

  DataServices data_services;
  data_services.session_service_ = session_service;
  EXPECT_CALL(*session_service, SubscribeSessionStateChanged(_));
  services.SetServices(std::move(data_services));

  promise<void> pending_connect;
  EXPECT_CALL(*session_service, Connect(_))
      .WillOnce([&](const scada::SessionConnectParams& params) {
        EXPECT_EQ(params.host, "node-host");
        return pending_connect;
      });

  auto result = ToPromise(
      MakeTestAnyExecutor(executor),
      services.coroutine_session_service().Connect({.host = "node-host"}));
  Drain(executor);

  EXPECT_EQ(result.wait_for(0ms), promise_wait_status::timeout);

  pending_connect.resolve();
  WaitPromise(executor, std::move(result));
}

TEST(MasterDataServicesTest, CoroutineSessionFacadeDelegatesSessionState) {
  auto executor = std::make_shared<TestExecutor>();
  MasterDataServices services{MakeTestAnyExecutor(executor)};
  auto session_service =
      std::make_shared<StrictMock<scada::MockSessionService>>();

  DataServices data_services;
  data_services.session_service_ = session_service;
  EXPECT_CALL(*session_service, SubscribeSessionStateChanged(_));
  services.SetServices(std::move(data_services));

  auto& coroutine_session = services.coroutine_session_service();
  base::TimeDelta ping_delay;

  EXPECT_CALL(*session_service, IsConnected(&ping_delay))
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*session_service, HasPrivilege(scada::Privilege::Configure))
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*session_service, GetUserId())
      .WillOnce(testing::Return(scada::NodeId{1, 2}));
  EXPECT_CALL(*session_service, GetHostName())
      .WillOnce(testing::Return("master-host"));
  EXPECT_CALL(*session_service, IsScada())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*session_service, GetSessionDebugger())
      .WillOnce(testing::Return(nullptr));

  EXPECT_TRUE(coroutine_session.IsConnected(&ping_delay));
  EXPECT_TRUE(coroutine_session.HasPrivilege(scada::Privilege::Configure));
  EXPECT_EQ(coroutine_session.GetUserId(), (scada::NodeId{1, 2}));
  EXPECT_EQ(coroutine_session.GetHostName(), "master-host");
  EXPECT_TRUE(coroutine_session.IsScada());
  EXPECT_EQ(coroutine_session.GetSessionDebugger(), nullptr);
}

}  // namespace
