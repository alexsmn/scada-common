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
