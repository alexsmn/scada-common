#include "base/test/test_executor.h"
#include "common/aliases_mock.h"
#include "common/node_event_provider_mock.h"
#include "core/attribute_service_mock.h"
#include "core/history_service_mock.h"
#include "core/method_service_mock.h"
#include "core/monitored_item_service_mock.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/node_model_mock.h"
#include "node_service/node_service_mock.h"
#include "timed_data/timed_data_service_impl.h"
#include "timed_data/timed_data_spec.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

class TimedDataTest : public Test {
 protected:
  std::shared_ptr<MockNodeModel> MakeTestNodeModel(
      const scada::NodeId& node_id) const;

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  StrictMock<MockAliasResolver> alias_resolver_;
  StrictMock<MockNodeService> node_service_;
  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockMethodService> method_service_;
  StrictMock<scada::MockMonitoredItemService> monitored_item_service_;
  StrictMock<scada::MockHistoryService> history_service_;
  NiceMock<MockNodeEventProvider> node_event_provider_;

  TimedDataServiceImpl service_{
      TimedDataContext{.executor_ = executor_,
                       .alias_resolver_ = alias_resolver_.AsStdFunction(),
                       .node_service_ = node_service_,
                       .attribute_service_ = attribute_service_,
                       .method_service_ = method_service_,
                       .monitored_item_service_ = monitored_item_service_,
                       .history_service_ = history_service_,
                       .node_event_provider_ = node_event_provider_}};
};

std::shared_ptr<MockNodeModel> TimedDataTest::MakeTestNodeModel(
    const scada::NodeId& node_id) const {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(node_id));
  return std::move(node_model);
}

TEST_F(TimedDataTest, TsFormat) {
  const scada::NodeId node_id{1, NamespaceIndexes::TS};
  const scada::NodeId ts_format_id{1, NamespaceIndexes::TS_FORMAT};
  const scada::LocalizedText close_label = u"Active";

  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();

  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(node_id));

  ON_CALL(*node_model,
          GetTarget(scada::NodeId{scada::id::HasTypeDefinition}, true))
      .WillByDefault(
          Return(NodeRef{MakeTestNodeModel(data_items::id::DiscreteItemType)}));

  auto ts_format = MakeTestNodeModel(ts_format_id);

  ON_CALL(*node_model, GetTarget(data_items::id::HasTsFormat, true))
      .WillByDefault(Return(NodeRef{ts_format}));

  auto ts_format_close_label = MakeTestNodeModel(ts_format_id);

  ON_CALL(*ts_format_close_label, GetAttribute(scada::AttributeId::Value))
      .WillByDefault(Return(close_label));

  ON_CALL(*ts_format, GetAggregate(data_items::id::TsFormatType_CloseLabel))
      .WillByDefault(Return(NodeRef{ts_format_close_label}));

  EXPECT_CALL(node_service_, GetNode(node_id))
      .WillOnce(Return(NodeRef{node_model}));

  auto monitored_item =
      std::make_shared<StrictMock<scada::MockMonitoredItem>>();

  EXPECT_CALL(*node_model, CreateMonitoredItem(_, _))
      .WillOnce(Return(monitored_item));

  const auto timestamp = scada::DateTime::Now();
  const scada::DataValue data_value{true, {}, timestamp, timestamp};

  EXPECT_CALL(*monitored_item,
              Subscribe(VariantWith<scada::DataChangeHandler>(_)))
      .WillOnce(
          Invoke([data_value](const scada::MonitoredItemHandler& handler) {
            std::get<scada::DataChangeHandler>(handler)(data_value);
          }));

  TimedDataSpec spec;
  spec.Connect(service_, NodeIdToScadaString(node_id));

  EXPECT_EQ(spec.GetCurrentString(), close_label);
}
