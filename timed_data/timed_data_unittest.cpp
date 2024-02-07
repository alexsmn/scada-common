#include "address_space/test/test_scada_node_states.h"
#include "base/test/test_executor.h"
#include "common/aliases_mock.h"
#include "common/variable_handle.h"
#include "events/node_event_provider_mock.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/node_model_mock.h"
#include "node_service/node_service_mock.h"
#include "node_service/static/static_node_service.h"
#include "scada/attribute_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "timed_data/timed_data_service_impl.h"
#include "timed_data/timed_data_spec.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

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
      {.executor_ = executor_,
       .alias_resolver_ = alias_resolver_.AsStdFunction(),
       .node_service_ = node_service_,
       .services_ = {.attribute_service = &attribute_service_,
                     .monitored_item_service = &monitored_item_service_,
                     .method_service = &method_service_,
                     .history_service = &history_service_},
       .node_event_provider_ = node_event_provider_}};

  const std::shared_ptr<scada::VariableHandle> node_value_variable_ =
      std::make_shared<scada::VariableHandle>();

  inline static const scada::NodeId node_id{1, NamespaceIndexes::TS};
  inline static const scada::NodeId ts_format_id{1,
                                                 NamespaceIndexes::TS_FORMAT};
  inline static const scada::LocalizedText close_label = u"Active";
};

TimedDataTest::TimedDataTest() {
  node_service_.AddAll(GetScadaNodeStates());

  node_service_.Add(
      {.node_id = ts_format_id,
       .type_definition_id = data_items::id::TsFormatType,
       .properties = {{data_items::id::TsFormatType_CloseLabel, close_label}}});

  node_service_.Add(
      {.node_id = node_id,
       .node_class = scada::NodeClass::Variable,
       .type_definition_id = data_items::id::DiscreteItemType,
       .references = {{.reference_type_id = data_items::id::HasTsFormat,
                       .node_id = ts_format_id}}});

  ON_CALL(monitored_item_service_, CreateMonitoredItem(_, _))
      .WillByDefault(Return(nullptr));

  ON_CALL(monitored_item_service_,
          CreateMonitoredItem(FieldsAre(node_id, scada::AttributeId::Value), _))
      .WillByDefault(
          Return(scada::CreateMonitoredVariable(node_value_variable_)));
}

TEST_F(TimedDataTest, TsFormat) {
  node_value_variable_->ForwardData(scada::MakeReadResult(true));

  TimedDataSpec spec{service_, node_id};

  EXPECT_EQ(spec.GetCurrentString(), close_label);
}

TEST_F(TimedDataTest, ExpressionVariableDeletes) {
  node_value_variable_->ForwardData(scada::MakeReadResult(123));

  const auto formula = std::format("{} + 55", NodeIdToScadaString(node_id));

  TimedDataSpec spec{service_, formula};

  EXPECT_EQ(spec.current().value, 123 + 55);
  EXPECT_TRUE(spec.current().qualifier.good());

  node_value_variable_->UpdateQualifier(0, scada::Qualifier::BAD);

  EXPECT_FALSE(spec.current().qualifier.good());

  node_value_variable_->UpdateQualifier(scada::Qualifier::BAD, 0);

  EXPECT_TRUE(spec.current().qualifier.good());
}
