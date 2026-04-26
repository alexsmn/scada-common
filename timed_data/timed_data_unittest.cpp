#include "address_space/test/test_scada_node_states.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/aliases_mock.h"
#include "common/history_util.h"
#include "common/variable_handle.h"
#include "events/node_event_provider_mock.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/node_model_mock.h"
#include "node_service/node_service_mock.h"
#include "node_service/static/static_node_service.h"
#include "scada/attribute_service_mock.h"
#include "scada/coroutine_services.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "timed_data/timed_data_service_factory.h"
#include "timed_data/timed_data_service_impl.h"
#include "timed_data/timed_data_spec.h"

#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

namespace {

template <typename T>
std::shared_ptr<T> UnownedService(T& service) {
  return std::shared_ptr<T>{&service, [](T*) {}};
}

DataServices MakeTimedDataServices(scada::HistoryService& history_service) {
  return {.history_service_ = UnownedService(history_service)};
}

class TestCoroutineHistoryService final
    : public scada::CoroutineHistoryService {
 public:
  Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override {
    ++raw_read_count;
    last_raw_details = std::move(details);
    co_return raw_result;
  }

  Awaitable<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId node_id,
      base::Time from,
      base::Time to,
      scada::EventFilter filter) override {
    co_return scada::HistoryReadEventsResult{.status = scada::StatusCode::Bad};
  }

  int raw_read_count = 0;
  scada::HistoryReadRawDetails last_raw_details;
  scada::HistoryReadRawResult raw_result{
      .status = scada::StatusCode::Good,
      .values = {},
  };
};

}  // namespace

class TimedDataTest : public Test {
 public:
  TimedDataTest();

 protected:
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  StrictMock<MockAliasResolver> alias_resolver_;
  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockMethodService> method_service_;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  StrictMock<scada::MockHistoryService> history_service_;
  NiceMock<MockNodeEventProvider> node_event_provider_;

  StaticNodeService node_service_{
      {.monitored_item_service = &monitored_item_service_}};

  TimedDataServiceImpl service_{
      {.executor_ = MakeTestAnyExecutor(executor_),
       .alias_resolver_ = alias_resolver_.AsStdFunction(),
       .node_service_ = node_service_,
       .data_services_ = MakeTimedDataServices(history_service_),
       .node_event_provider_ = node_event_provider_}};

  const std::shared_ptr<scada::VariableHandle> node_value_variable_ =
      std::make_shared<scada::VariableHandle>();

  StrictMock<MockFunction<void(const PropertySet& properties)>>
      property_change_handler_;

  inline static const scada::NodeId kDataItemId{1, NamespaceIndexes::TS};
  inline static const std::string_view kDataItemAlias = "Alias";

  inline static const scada::NodeId kTsFormatId{1, NamespaceIndexes::TS_FORMAT};
  inline static const scada::LocalizedText kFormatCloseLabel = u"Active";
};

TimedDataTest::TimedDataTest() {
  node_service_.AddAll(GetScadaNodeStates());

  node_service_.Add({.node_id = kTsFormatId,
                     .type_definition_id = data_items::id::TsFormatType,
                     .properties = {{data_items::id::TsFormatType_CloseLabel,
                                     kFormatCloseLabel}}});

  node_service_.Add(
      {.node_id = kDataItemId,
       .node_class = scada::NodeClass::Variable,
       .type_definition_id = data_items::id::DiscreteItemType,
       .properties = {{data_items::id::DataItemType_Alias,
                       scada::String{kDataItemAlias}}},
       .references = {{.reference_type_id = data_items::id::HasTsFormat,
                       .node_id = kTsFormatId}}});

  ON_CALL(monitored_item_service_,
          CreateMonitoredItem(/*read_value_id=*/_, /*params=*/_))
      .WillByDefault(Return(nullptr));

  ON_CALL(
      monitored_item_service_,
      CreateMonitoredItem(FieldsAre(kDataItemId, scada::AttributeId::Value), _))
      .WillByDefault(
          Return(scada::CreateMonitoredVariable(node_value_variable_)));
}

TEST_F(TimedDataTest, NodeTsFormat) {
  node_value_variable_->ForwardData(scada::MakeReadResult(true));

  TimedDataSpec spec{service_, kDataItemId};

  EXPECT_TRUE(spec.connected());
  EXPECT_EQ(spec.GetCurrentString(), kFormatCloseLabel);
}

TEST_F(TimedDataTest, AliasTsFormat) {
  node_value_variable_->ForwardData(scada::MakeReadResult(true));

  EXPECT_CALL(alias_resolver_, Call(kDataItemAlias, /*callback=*/_))
      .WillOnce(InvokeArgument<1>(scada::StatusCode::Good, kDataItemId));

  TimedDataSpec spec{service_, kDataItemAlias};

  EXPECT_TRUE(spec.connected());
  EXPECT_EQ(spec.GetCurrentString(), kFormatCloseLabel);
}

TEST_F(TimedDataTest, AliasValueUpdatesAfterAliasResolution) {
  node_value_variable_->ForwardData(scada::MakeReadResult(111));

  AliasResolveCallback alias_resolve_callback;

  EXPECT_CALL(alias_resolver_, Call(kDataItemAlias, /*callback=*/_))
      .WillOnce(SaveArg<1>(&alias_resolve_callback));

  TimedDataSpec spec{service_, kDataItemAlias};
  spec.property_change_handler = property_change_handler_.AsStdFunction();

  EXPECT_CALL(property_change_handler_, Call(/*props=*/_)).WillOnce(Invoke([&] {
    EXPECT_EQ(spec.current().value, 111);
  }));

  alias_resolve_callback(scada::StatusCode::Good, kDataItemId);

  EXPECT_CALL(property_change_handler_, Call(/*props=*/_)).WillOnce(Invoke([&] {
    EXPECT_EQ(spec.current().value, 222);
  }));

  node_value_variable_->ForwardData(scada::MakeReadResult(222));
}

TEST_F(TimedDataTest, ExpressionVariableDeletes) {
  node_value_variable_->ForwardData(scada::MakeReadResult(123));

  const auto formula = std::format("{} + 55", NodeIdToScadaString(kDataItemId));

  TimedDataSpec spec{service_, formula};

  EXPECT_TRUE(spec.connected());
  EXPECT_EQ(spec.current().value, 123 + 55);
  EXPECT_TRUE(spec.current().qualifier.good());

  node_value_variable_->UpdateQualifier(0, scada::Qualifier::BAD);

  EXPECT_FALSE(spec.current().qualifier.good());

  node_value_variable_->UpdateQualifier(scada::Qualifier::BAD, 0);

  EXPECT_TRUE(spec.current().qualifier.good());
}

TEST_F(TimedDataTest, HistoryFetchUsesServiceLevelCoroutineAdapter) {
  const auto from = base::Time::Now();
  const auto to = from + base::TimeDelta::FromSeconds(10);

  EXPECT_CALL(history_service_, HistoryReadRaw(_, _))
      .WillOnce(Invoke([&](const scada::HistoryReadRawDetails& details,
                           const scada::HistoryReadRawCallback& callback) {
        EXPECT_EQ(details.node_id, kDataItemId);
        EXPECT_EQ(details.from, from);
        EXPECT_EQ(details.to, to);
        callback(scada::HistoryReadRawResult{
            .status = scada::StatusCode::Good,
            .values = {},
        });
      }));

  TimedDataSpec spec{service_, kDataItemId};
  spec.SetRange({from, to});

  Drain(executor_);

  EXPECT_TRUE(spec.range_ready({from, to}));
}

TEST_F(TimedDataTest, DataServicesHistoryCallbackUsesCoroutineAdapter) {
  const auto from = base::Time::Now();
  const auto to = from + base::TimeDelta::FromSeconds(10);
  auto service = CreateTimedDataService(TimedDataContext{
      .executor_ = MakeTestAnyExecutor(executor_),
      .alias_resolver_ = alias_resolver_.AsStdFunction(),
      .node_service_ = node_service_,
      .data_services_ = MakeTimedDataServices(history_service_),
      .node_event_provider_ = node_event_provider_});

  EXPECT_CALL(history_service_, HistoryReadRaw(_, _))
      .WillOnce(Invoke([&](const scada::HistoryReadRawDetails& details,
                           const scada::HistoryReadRawCallback& callback) {
        EXPECT_EQ(details.node_id, kDataItemId);
        EXPECT_EQ(details.from, from);
        EXPECT_EQ(details.to, to);
        executor_->PostTask([callback] {
          callback(scada::HistoryReadRawResult{
              .status = scada::StatusCode::Good,
              .values = {},
          });
        });
      }));

  TimedDataSpec spec{*service, kDataItemId};
  spec.SetRange({from, to});

  Drain(executor_);

  EXPECT_TRUE(spec.range_ready({from, to}));
}

TEST_F(TimedDataTest, HistoryFetchUsesCoroutineFactoryContext) {
  const auto from = base::Time::Now();
  const auto to = from + base::TimeDelta::FromSeconds(10);
  auto history_service = std::make_shared<TestCoroutineHistoryService>();
  auto service = CreateTimedDataService(CoroutineTimedDataContext{
      .executor_ = MakeTestAnyExecutor(executor_),
      .alias_resolver_ = alias_resolver_.AsStdFunction(),
      .node_service_ = node_service_,
      .history_service_ = history_service,
      .node_event_provider_ = node_event_provider_});

  TimedDataSpec spec{*service, kDataItemId};
  spec.SetRange({from, to});

  Drain(executor_);

  EXPECT_EQ(history_service->raw_read_count, 1);
  EXPECT_EQ(history_service->last_raw_details.node_id, kDataItemId);
  EXPECT_EQ(history_service->last_raw_details.from, from);
  EXPECT_EQ(history_service->last_raw_details.to, to);
  EXPECT_TRUE(spec.range_ready({from, to}));
}

TEST_F(TimedDataTest, HistoryFetchUsesDataServicesCoroutineSlot) {
  const auto from = base::Time::Now();
  const auto to = from + base::TimeDelta::FromSeconds(10);
  auto history_service = std::make_shared<TestCoroutineHistoryService>();

  DataServices data_services;
  data_services.coroutine_history_service_ = history_service;

  auto service = CreateTimedDataService(
      TimedDataContext{.executor_ = MakeTestAnyExecutor(executor_),
                       .alias_resolver_ = alias_resolver_.AsStdFunction(),
                       .node_service_ = node_service_,
                       .data_services_ = std::move(data_services),
                       .node_event_provider_ = node_event_provider_});

  TimedDataSpec spec{*service, kDataItemId};
  spec.SetRange({from, to});

  Drain(executor_);

  EXPECT_EQ(history_service->raw_read_count, 1);
  EXPECT_EQ(history_service->last_raw_details.node_id, kDataItemId);
  EXPECT_EQ(history_service->last_raw_details.from, from);
  EXPECT_EQ(history_service->last_raw_details.to, to);
  EXPECT_TRUE(spec.range_ready({from, to}));
}

TEST_F(TimedDataTest, ScopedContinuationPointReleasesThroughCoroutineCleanup) {
  const scada::HistoryReadRawDetails details{
      .node_id = kDataItemId,
      .from = base::Time::Now(),
      .to = base::Time::Now() + base::TimeDelta::FromSeconds(1),
      .max_count = 100,
  };
  const scada::ByteString continuation_point{'c', 'p'};

  EXPECT_CALL(history_service_, HistoryReadRaw(_, _))
      .WillOnce(Invoke([&](const scada::HistoryReadRawDetails& cleanup,
                           const scada::HistoryReadRawCallback& callback) {
        EXPECT_EQ(cleanup.node_id, details.node_id);
        EXPECT_EQ(cleanup.from, details.from);
        EXPECT_EQ(cleanup.to, details.to);
        EXPECT_EQ(cleanup.max_count, details.max_count);
        EXPECT_TRUE(cleanup.release_continuation_point);
        EXPECT_EQ(cleanup.continuation_point, continuation_point);
        callback({});
      }));

  scada::CallbackToCoroutineHistoryServiceAdapter history_service_adapter{
      MakeTestAnyExecutor(executor_), history_service_};
  ScopedContinuationPoint scoped_continuation_point{
      MakeTestAnyExecutor(executor_), history_service_adapter, details,
      continuation_point};
  scoped_continuation_point.reset();

  Drain(executor_);
}

TEST_F(TimedDataTest, ScopedContinuationPointReleaseSkipsCleanup) {
  const scada::HistoryReadRawDetails details{
      .node_id = kDataItemId,
      .from = base::Time::Now(),
      .to = base::Time::Now() + base::TimeDelta::FromSeconds(1),
      .max_count = 100,
  };
  const scada::ByteString continuation_point{'c', 'p'};

  EXPECT_CALL(history_service_, HistoryReadRaw(_, _)).Times(0);

  scada::CallbackToCoroutineHistoryServiceAdapter history_service_adapter{
      MakeTestAnyExecutor(executor_), history_service_};
  ScopedContinuationPoint scoped_continuation_point{
      MakeTestAnyExecutor(executor_), history_service_adapter, details,
      continuation_point};
  EXPECT_EQ(scoped_continuation_point.release(), continuation_point);
  scoped_continuation_point.reset();

  Drain(executor_);
}
