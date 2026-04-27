#include "common/master_data_services.h"

#include "base/awaitable_promise.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/coroutine_service_resolver.h"
#include "common/data_services_util.h"
#include "scada/attribute_service_mock.h"
#include "scada/coroutine_services.h"
#include "scada/node_management_service_mock.h"
#include "scada/session_service_mock.h"
#include "scada/test/status_matchers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <vector>

namespace {

using namespace std::chrono_literals;
using testing::_;
using testing::StrictMock;

class TestCoroutineDataServices final
    : public scada::CoroutineSessionService,
      public scada::CoroutineAttributeService,
      public scada::CoroutineViewService,
      public scada::CoroutineMethodService,
      public scada::CoroutineHistoryService,
      public scada::CoroutineNodeManagementService {
 public:
  Awaitable<void> Connect(scada::SessionConnectParams params) override {
    ++connect_count;
    last_host = std::move(params.host);
    co_return;
  }

  Awaitable<void> Reconnect() override {
    ++reconnect_count;
    co_return;
  }

  Awaitable<void> Disconnect() override {
    ++disconnect_count;
    co_return;
  }

  bool IsConnected(base::TimeDelta* /*ping_delay*/ = nullptr) const override {
    return connected;
  }

  scada::NodeId GetUserId() const override { return user_id; }

  bool HasPrivilege(scada::Privilege privilege) const override {
    return privilege == scada::Privilege::Configure;
  }

  std::string GetHostName() const override { return host_name; }

  bool IsScada() const override { return is_scada; }

  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override {
    ++session_subscription_count;
    return session_state_changed.connect(callback);
  }

  scada::SessionDebugger* GetSessionDebugger() override { return nullptr; }

  Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> Read(
      scada::ServiceContext context,
      std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) override {
    ++read_count;
    last_read_context = std::move(context);
    last_read_inputs = *inputs;
    co_return std::vector<scada::DataValue>{read_value};
  }

  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> Write(
      scada::ServiceContext /*context*/,
      std::shared_ptr<const std::vector<scada::WriteValue>> inputs) override {
    ++write_count;
    co_return std::vector<scada::StatusCode>(inputs->size(),
                                             scada::StatusCode::Good);
  }

  Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
  Browse(scada::ServiceContext /*context*/,
         std::vector<scada::BrowseDescription> inputs) override {
    ++browse_count;
    last_browse_inputs = std::move(inputs);
    co_return std::vector<scada::BrowseResult>{
        {.references = {browse_reference}}};
  }

  Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override {
    ++translate_count;
    co_return std::vector<scada::BrowsePathResult>(inputs.size());
  }

  Awaitable<scada::Status> Call(scada::NodeId node_id,
                                scada::NodeId method_id,
                                std::vector<scada::Variant> /*arguments*/,
                                scada::NodeId user_id) override {
    ++call_count;
    last_call_node_id = std::move(node_id);
    last_call_method_id = std::move(method_id);
    last_call_user_id = std::move(user_id);
    co_return scada::Status{scada::StatusCode::Good};
  }

  Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override {
    ++history_raw_count;
    last_history_raw_details = std::move(details);
    co_return scada::HistoryReadRawResult{
        .status = scada::Status{scada::StatusCode::Good},
        .values = {read_value}};
  }

  Awaitable<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId node_id,
      base::Time /*from*/,
      base::Time /*to*/,
      scada::EventFilter /*filter*/) override {
    ++history_events_count;
    last_history_events_node_id = std::move(node_id);
    co_return scada::HistoryReadEventsResult{
        .status = scada::Status{scada::StatusCode::Good}};
  }

  Awaitable<scada::StatusOr<std::vector<scada::AddNodesResult>>>
  AddNodes(std::vector<scada::AddNodesItem> inputs) override {
    ++add_nodes_count;
    last_add_nodes_inputs = std::move(inputs);
    co_return std::vector<scada::AddNodesResult>{
        {.added_node_id = scada::NodeId{700, 7}}};
  }

  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
  DeleteNodes(std::vector<scada::DeleteNodesItem> inputs) override {
    ++delete_nodes_count;
    co_return std::vector<scada::StatusCode>(inputs.size(),
                                             scada::StatusCode::Good);
  }

  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
  AddReferences(std::vector<scada::AddReferencesItem> inputs) override {
    ++add_references_count;
    co_return std::vector<scada::StatusCode>(inputs.size(),
                                             scada::StatusCode::Good);
  }

  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
  DeleteReferences(std::vector<scada::DeleteReferencesItem> inputs) override {
    ++delete_references_count;
    co_return std::vector<scada::StatusCode>(inputs.size(),
                                             scada::StatusCode::Good);
  }

  bool connected = true;
  bool is_scada = true;
  std::string host_name = "direct-host";
  scada::NodeId user_id{55, 5};
  scada::DataValue read_value;
  scada::ReferenceDescription browse_reference{.node_id = scada::NodeId{900}};
  boost::signals2::signal<void(bool, const scada::Status&)>
      session_state_changed;

  int connect_count = 0;
  int reconnect_count = 0;
  int disconnect_count = 0;
  int session_subscription_count = 0;
  int read_count = 0;
  int write_count = 0;
  int browse_count = 0;
  int translate_count = 0;
  int call_count = 0;
  int history_raw_count = 0;
  int history_events_count = 0;
  int add_nodes_count = 0;
  int delete_nodes_count = 0;
  int add_references_count = 0;
  int delete_references_count = 0;

  std::string last_host;
  scada::ServiceContext last_read_context;
  std::vector<scada::ReadValueId> last_read_inputs;
  std::vector<scada::BrowseDescription> last_browse_inputs;
  scada::NodeId last_call_node_id;
  scada::NodeId last_call_method_id;
  scada::NodeId last_call_user_id;
  scada::HistoryReadRawDetails last_history_raw_details;
  scada::NodeId last_history_events_node_id;
  std::vector<scada::AddNodesItem> last_add_nodes_inputs;
};

TEST(DataServicesUtilTest, HasServicesDetectsCallbackAndCoroutineSlots) {
  EXPECT_FALSE(data_services::HasServices(DataServices{}));

  StrictMock<scada::MockAttributeService> attribute_service;
  DataServices callback_services;
  callback_services.attribute_service_ =
      std::shared_ptr<scada::AttributeService>{&attribute_service,
                                               [](scada::AttributeService*) {}};
  EXPECT_TRUE(data_services::HasServices(callback_services));

  DataServices coroutine_services;
  coroutine_services.coroutine_method_service_ =
      std::make_shared<TestCoroutineDataServices>();
  EXPECT_TRUE(data_services::HasServices(coroutine_services));
}

TEST(DataServicesUtilTest, FromUnownedServicesAliasesLegacySlots) {
  StrictMock<scada::MockAttributeService> attribute_service;
  StrictMock<scada::MockSessionService> session_service;

  auto data_services = data_services::FromUnownedServices(scada::services{
      .attribute_service = &attribute_service,
      .session_service = &session_service});

  EXPECT_TRUE(data_services::HasServices(data_services));
  EXPECT_EQ(data_services.attribute_service_.get(), &attribute_service);
  EXPECT_EQ(data_services.session_service_.get(), &session_service);
}

TEST(DataServicesUtilTest, UnownedAliasesServiceWithoutOwningIt) {
  StrictMock<scada::MockAttributeService> attribute_service;

  auto service = data_services::Unowned(attribute_service);

  EXPECT_EQ(service.get(), &attribute_service);
  EXPECT_FALSE(service.owner_before(std::shared_ptr<void>{}));
  EXPECT_FALSE(std::shared_ptr<void>{}.owner_before(service));
}

TEST(CoroutineServiceResolverTest, OptionalExecutorDoesNotCreateAdapter) {
  auto attribute_service =
      std::make_shared<StrictMock<scada::MockAttributeService>>();
  std::unique_ptr<scada::CallbackToCoroutineAttributeServiceAdapter> adapter;

  auto* resolved = scada::service_resolver::ResolveCoroutineService(
      std::optional<AnyExecutor>{},
      std::shared_ptr<scada::CoroutineAttributeService>{}, attribute_service,
      adapter);

  EXPECT_EQ(resolved, nullptr);
  EXPECT_EQ(adapter, nullptr);
}

TEST(CoroutineServiceResolverTest, CreatesCallbackAdapterForResolvedService) {
  auto executor = std::make_shared<TestExecutor>();
  auto coroutine_services = std::make_shared<TestCoroutineDataServices>();
  std::unique_ptr<scada::CallbackToCoroutineAttributeServiceAdapter>
      callback_to_coroutine_adapter;
  std::unique_ptr<scada::CoroutineToCallbackAttributeServiceAdapter>
      coroutine_to_callback_adapter;

  auto* resolved = scada::service_resolver::ResolveCoroutineService(
      std::optional<AnyExecutor>{MakeTestAnyExecutor(executor)},
      std::shared_ptr<scada::CoroutineAttributeService>{coroutine_services},
      std::shared_ptr<scada::AttributeService>{},
      callback_to_coroutine_adapter, coroutine_to_callback_adapter);

  EXPECT_EQ(resolved, coroutine_services.get());
  EXPECT_EQ(callback_to_coroutine_adapter, nullptr);
  ASSERT_NE(coroutine_to_callback_adapter, nullptr);

  bool read_called = false;
  coroutine_to_callback_adapter->Read(
      {}, std::make_shared<const std::vector<scada::ReadValueId>>(),
      [&](scada::Status status, std::vector<scada::DataValue> values) {
        read_called = true;
        EXPECT_TRUE(status.good());
        EXPECT_EQ(values.size(), 1u);
      });
  Drain(executor);

  EXPECT_TRUE(read_called);
  EXPECT_EQ(coroutine_services->read_count, 1);
}

TEST(CoroutineServiceResolverTest, SharedResolverCreatesCallbackAdapter) {
  auto executor = std::make_shared<TestExecutor>();
  auto attribute_service =
      std::make_shared<StrictMock<scada::MockAttributeService>>();

  auto resolved =
      scada::service_resolver::ResolveCoroutineServiceShared<
          scada::CoroutineAttributeService, scada::AttributeService,
          scada::CallbackToCoroutineAttributeServiceAdapter>(
          MakeTestAnyExecutor(executor),
          std::shared_ptr<scada::CoroutineAttributeService>{},
          attribute_service);

  ASSERT_NE(resolved, nullptr);

  scada::ReadCallback pending_read;
  EXPECT_CALL(*attribute_service, Read(_, _, _))
      .WillOnce(
          [&](const scada::ServiceContext&,
              const std::shared_ptr<const std::vector<scada::ReadValueId>>&,
              const scada::ReadCallback& callback) {
            pending_read = callback;
          });

  auto result = ToPromise(
      MakeTestAnyExecutor(executor),
      resolved->Read({}, std::make_shared<const std::vector<scada::ReadValueId>>()));
  Drain(executor);

  EXPECT_EQ(result.wait_for(0ms), promise_wait_status::timeout);
  ASSERT_TRUE(pending_read);

  pending_read(scada::StatusCode::Good, {scada::DataValue{}});

  ASSERT_OK_AND_ASSIGN(auto values, WaitPromise(executor, std::move(result)));
  EXPECT_EQ(values.size(), 1u);
}

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

  ASSERT_OK_AND_ASSIGN(auto results,
                       WaitPromise(executor, std::move(result)));
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

TEST(MasterDataServicesTest, DataServicesCoroutineSlotsDriveAggregateApis) {
  auto executor = std::make_shared<TestExecutor>();
  MasterDataServices services{MakeTestAnyExecutor(executor)};
  auto direct_services = std::make_shared<TestCoroutineDataServices>();

  DataServices data_services;
  data_services.coroutine_session_service_ = direct_services;
  data_services.coroutine_attribute_service_ = direct_services;
  data_services.coroutine_view_service_ = direct_services;
  data_services.coroutine_method_service_ = direct_services;
  data_services.coroutine_history_service_ = direct_services;
  data_services.coroutine_node_management_service_ = direct_services;
  services.SetServices(std::move(data_services));

  EXPECT_EQ(direct_services->session_subscription_count, 1);
  EXPECT_TRUE(services.IsConnected());
  EXPECT_TRUE(services.HasPrivilege(scada::Privilege::Configure));
  EXPECT_EQ(services.GetUserId(), (scada::NodeId{55, 5}));
  EXPECT_EQ(services.GetHostName(), "direct-host");
  EXPECT_TRUE(services.IsScada());

  WaitPromise(executor, services.Connect({.host = "direct-connect"}));
  EXPECT_EQ(direct_services->connect_count, 1);
  EXPECT_EQ(direct_services->last_host, "direct-connect");

  bool read_called = false;
  auto read_inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      std::vector<scada::ReadValueId>{{.node_id = scada::NodeId{100}}});
  services.Read({}, read_inputs,
                [&](scada::Status status,
                    std::vector<scada::DataValue> results) {
                  read_called = true;
                  EXPECT_TRUE(status.good());
                  ASSERT_EQ(results.size(), 1u);
                  EXPECT_EQ(results[0], direct_services->read_value);
                });
  Drain(executor);

  EXPECT_TRUE(read_called);
  EXPECT_EQ(direct_services->read_count, 1);
  EXPECT_EQ(direct_services->last_read_inputs, *read_inputs);

  bool write_called = false;
  auto write_inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      std::vector<scada::WriteValue>{});
  services.Write({}, write_inputs,
                 [&](scada::Status status,
                     std::vector<scada::StatusCode> results) {
                   write_called = true;
                   EXPECT_TRUE(status.good());
                   EXPECT_TRUE(results.empty());
                 });
  Drain(executor);

  EXPECT_TRUE(write_called);
  EXPECT_EQ(direct_services->write_count, 1);

  bool browse_called = false;
  services.Browse(
      {}, {{.node_id = scada::NodeId{106}}},
      [&](scada::Status status, std::vector<scada::BrowseResult> results) {
        browse_called = true;
        EXPECT_TRUE(status.good());
        ASSERT_EQ(results.size(), 1u);
        ASSERT_EQ(results[0].references.size(), 1u);
        EXPECT_EQ(results[0].references[0].node_id, (scada::NodeId{900}));
      });
  Drain(executor);

  EXPECT_TRUE(browse_called);
  EXPECT_EQ(direct_services->browse_count, 1);
  ASSERT_EQ(direct_services->last_browse_inputs.size(), 1u);
  EXPECT_EQ(direct_services->last_browse_inputs[0].node_id,
            (scada::NodeId{106}));

  bool translate_called = false;
  services.TranslateBrowsePaths(
      {scada::BrowsePath{}},
      [&](scada::Status status,
          std::vector<scada::BrowsePathResult> results) {
        translate_called = true;
        EXPECT_TRUE(status.good());
        EXPECT_EQ(results.size(), 1u);
      });
  Drain(executor);

  EXPECT_TRUE(translate_called);
  EXPECT_EQ(direct_services->translate_count, 1);

  bool add_nodes_called = false;
  services.AddNodes(
      {{.requested_id = scada::NodeId{101}}},
      [&](scada::Status status, std::vector<scada::AddNodesResult> results) {
        add_nodes_called = true;
        EXPECT_TRUE(status.good());
        ASSERT_EQ(results.size(), 1u);
        EXPECT_EQ(results[0].added_node_id, (scada::NodeId{700, 7}));
      });
  Drain(executor);

  EXPECT_TRUE(add_nodes_called);
  EXPECT_EQ(direct_services->add_nodes_count, 1);
  ASSERT_EQ(direct_services->last_add_nodes_inputs.size(), 1u);
  EXPECT_EQ(direct_services->last_add_nodes_inputs[0].requested_id,
            (scada::NodeId{101}));

  bool delete_nodes_called = false;
  services.DeleteNodes(
      {scada::DeleteNodesItem{}},
      [&](scada::Status status, std::vector<scada::StatusCode> results) {
        delete_nodes_called = true;
        EXPECT_TRUE(status.good());
        EXPECT_EQ(results.size(), 1u);
      });
  Drain(executor);

  EXPECT_TRUE(delete_nodes_called);
  EXPECT_EQ(direct_services->delete_nodes_count, 1);

  bool add_references_called = false;
  services.AddReferences(
      {scada::AddReferencesItem{}},
      [&](scada::Status status, std::vector<scada::StatusCode> results) {
        add_references_called = true;
        EXPECT_TRUE(status.good());
        EXPECT_EQ(results.size(), 1u);
      });
  Drain(executor);

  EXPECT_TRUE(add_references_called);
  EXPECT_EQ(direct_services->add_references_count, 1);

  bool delete_references_called = false;
  services.DeleteReferences(
      {scada::DeleteReferencesItem{}},
      [&](scada::Status status, std::vector<scada::StatusCode> results) {
        delete_references_called = true;
        EXPECT_TRUE(status.good());
        EXPECT_EQ(results.size(), 1u);
      });
  Drain(executor);

  EXPECT_TRUE(delete_references_called);
  EXPECT_EQ(direct_services->delete_references_count, 1);

  bool call_called = false;
  services.Call(scada::NodeId{102}, scada::NodeId{103}, {}, scada::NodeId{104},
                [&](scada::Status status) {
                  call_called = true;
                  EXPECT_TRUE(status.good());
                });
  Drain(executor);

  EXPECT_TRUE(call_called);
  EXPECT_EQ(direct_services->call_count, 1);
  EXPECT_EQ(direct_services->last_call_node_id, (scada::NodeId{102}));
  EXPECT_EQ(direct_services->last_call_method_id, (scada::NodeId{103}));
  EXPECT_EQ(direct_services->last_call_user_id, (scada::NodeId{104}));

  bool history_called = false;
  const scada::HistoryReadRawDetails history_details{
      .node_id = scada::NodeId{105}, .max_count = 3};
  services.HistoryReadRaw(history_details,
                          [&](scada::HistoryReadRawResult result) {
                            history_called = true;
                            EXPECT_TRUE(result.status.good());
                            ASSERT_EQ(result.values.size(), 1u);
                            EXPECT_EQ(result.values[0],
                                      direct_services->read_value);
                          });
  Drain(executor);

  EXPECT_TRUE(history_called);
  EXPECT_EQ(direct_services->history_raw_count, 1);
  EXPECT_EQ(direct_services->last_history_raw_details.node_id,
            history_details.node_id);
  EXPECT_EQ(direct_services->last_history_raw_details.max_count,
            history_details.max_count);

  bool history_events_called = false;
  services.HistoryReadEvents(
      scada::NodeId{107}, base::Time{}, base::Time{}, scada::EventFilter{},
      [&](scada::HistoryReadEventsResult result) {
        history_events_called = true;
        EXPECT_TRUE(result.status.good());
      });
  Drain(executor);

  EXPECT_TRUE(history_events_called);
  EXPECT_EQ(direct_services->history_events_count, 1);
  EXPECT_EQ(direct_services->last_history_events_node_id,
            (scada::NodeId{107}));

  ASSERT_OK_AND_ASSIGN(auto browse_results, WaitAwaitable(
      executor, static_cast<scada::CoroutineViewService&>(services).Browse(
                    {}, {{.node_id = scada::NodeId{108}}})));
  ASSERT_EQ(browse_results.size(), 1u);
  ASSERT_EQ(browse_results[0].references.size(), 1u);
  EXPECT_EQ(browse_results[0].references[0].node_id, (scada::NodeId{900}));
  EXPECT_EQ(direct_services->browse_count, 2);
  ASSERT_EQ(direct_services->last_browse_inputs.size(), 1u);
  EXPECT_EQ(direct_services->last_browse_inputs[0].node_id,
            (scada::NodeId{108}));
}

}  // namespace
