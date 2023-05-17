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
#include "node_service/static/static_node_service.h"
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
  StrictMock<scada::MockMonitoredItemService> monitored_item_service_;
  StrictMock<scada::MockHistoryService> history_service_;
  NiceMock<MockNodeEventProvider> node_event_provider_;

  StaticNodeService node_service_;

  TimedDataServiceImpl service_{
      TimedDataContext{.executor_ = executor_,
                       .alias_resolver_ = alias_resolver_.AsStdFunction(),
                       .node_service_ = node_service_,
                       .attribute_service_ = attribute_service_,
                       .method_service_ = method_service_,
                       .monitored_item_service_ = monitored_item_service_,
                       .history_service_ = history_service_,
                       .node_event_provider_ = node_event_provider_}};

  inline static const scada::NodeId node_id{1, NamespaceIndexes::TS};
  inline static const scada::NodeId ts_format_id{1,
                                                 NamespaceIndexes::TS_FORMAT};
  inline static const scada::LocalizedText close_label = u"Active";
};

TimedDataTest::TimedDataTest() {
  node_service_.Add(
      {.node_id = ts_format_id,
       .type_definition_id = data_items::id::TsFormatType,
       .properties = {{data_items::id::TsFormatType_CloseLabel, close_label}}});

  node_service_.Add(
      {.node_id = node_id,
       .type_definition_id = data_items::id::DiscreteItemType,
       .attributes = {.value = true},
       .references = {{data_items::id::HasTsFormat, true, ts_format_id}}});
}

TEST_F(TimedDataTest, TsFormat) {
  TimedDataSpec spec;
  spec.Connect(service_, NodeIdToScadaString(node_id));

  EXPECT_EQ(spec.GetCurrentString(), close_label);
}

TEST_F(TimedDataTest, ExpressionVariableDeletes) {
  TimedDataSpec spec;
  spec.Connect(service_, NodeIdToScadaString(node_id));
}
