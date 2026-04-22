#include "opcua/opcua_endpoint_core.h"

#include "scada/attribute_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/monitoring_parameters.h"
#include "scada/node_management_service_mock.h"
#include "scada/test/test_monitored_item.h"
#include "scada/view_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace scada::opcua_endpoint {
namespace {

class OpcUaEndpointCoreTest : public Test {
 protected:
  static scada::NodeId NumericNode(scada::NumericId id,
                                   scada::NamespaceIndex ns = 2) {
    return {id, ns};
  }

  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockMethodService> method_service_;
  StrictMock<scada::MockMonitoredItemService> monitored_item_service_;
  StrictMock<scada::MockNodeManagementService> node_management_service_;
  StrictMock<scada::MockViewService> view_service_;
};

TEST_F(OpcUaEndpointCoreTest, MakeServiceContext_SetsRequestedUserId) {
  const auto user_id = NumericNode(42, 3);
  const auto context = MakeServiceContext(user_id);
  EXPECT_EQ(context.user_id(), user_id);
}

TEST_F(OpcUaEndpointCoreTest, Read_NormalizesBadWrongNodeIdInCallbackPath) {
  const auto user_id = NumericNode(99, 4);
  auto inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      std::vector<scada::ReadValueId>{
          {.node_id = NumericNode(1), .attribute_id = scada::AttributeId::Value}});

  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& actual_context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& actual_inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(actual_context.user_id(), user_id);
        EXPECT_EQ(actual_inputs, inputs);
        callback(scada::StatusCode::Good,
                 {scada::MakeReadError(scada::StatusCode::Bad_WrongNodeId)});
      }));

  std::optional<scada::Status> status;
  std::optional<std::vector<scada::DataValue>> results;
  Read(attribute_service_, MakeServiceContext(user_id), inputs,
       [&](scada::Status actual_status,
           std::vector<scada::DataValue> actual_results) {
         status = std::move(actual_status);
         results = std::move(actual_results);
       });

  ASSERT_TRUE(status.has_value());
  ASSERT_TRUE(results.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Good);
  ASSERT_EQ(results->size(), 1u);
  EXPECT_EQ(scada::Status((*results)[0].status_code).full_code(), 0x80340000u);
}

TEST_F(OpcUaEndpointCoreTest, Write_ForwardsUserContextAndResults) {
  const auto user_id = NumericNode(101, 4);
  auto inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      std::vector<scada::WriteValue>{{
          .node_id = NumericNode(3),
          .attribute_id = scada::AttributeId::Value,
          .value = scada::Variant{scada::Int32{7}},
      }});

  EXPECT_CALL(attribute_service_, Write(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& actual_context,
                           const std::shared_ptr<const std::vector<scada::WriteValue>>& actual_inputs,
                           const scada::WriteCallback& callback) {
        EXPECT_EQ(actual_context.user_id(), user_id);
        EXPECT_EQ(actual_inputs, inputs);
        callback(scada::StatusCode::Good, {scada::StatusCode::Good});
      }));

  std::optional<scada::Status> status;
  std::optional<std::vector<scada::StatusCode>> results;
  Write(attribute_service_, MakeServiceContext(user_id), inputs,
        [&](scada::Status actual_status,
            std::vector<scada::StatusCode> actual_results) {
          status = std::move(actual_status);
          results = std::move(actual_results);
        });

  ASSERT_TRUE(status.has_value());
  ASSERT_TRUE(results.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Good);
  EXPECT_THAT(*results, ElementsAre(scada::StatusCode::Good));
}

TEST_F(OpcUaEndpointCoreTest, Browse_ForwardsContextInputsAndResults) {
  const auto user_id = NumericNode(55, 6);
  const auto inputs = std::vector<scada::BrowseDescription>{
      {.node_id = NumericNode(5),
       .direction = scada::BrowseDirection::Forward,
       .reference_type_id = NumericNode(9),
       .include_subtypes = false}};

  EXPECT_CALL(view_service_, Browse(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& actual_context,
                           const std::vector<scada::BrowseDescription>& actual_inputs,
                           const scada::BrowseCallback& callback) {
        EXPECT_EQ(actual_context.user_id(), user_id);
        EXPECT_THAT(actual_inputs, ElementsAre(inputs[0]));
        callback(scada::StatusCode::Good,
                 {scada::BrowseResult{
                     .status_code = scada::StatusCode::Good,
                     .references = {{.reference_type_id = NumericNode(10),
                                     .forward = true,
                                     .node_id = NumericNode(11)}}}});
      }));

  std::optional<scada::Status> status;
  std::optional<std::vector<scada::BrowseResult>> results;
  Browse(view_service_, MakeServiceContext(user_id), inputs,
         [&](scada::Status actual_status,
             std::vector<scada::BrowseResult> actual_results) {
           status = std::move(actual_status);
           results = std::move(actual_results);
         });

  ASSERT_TRUE(status.has_value());
  ASSERT_TRUE(results.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Good);
  ASSERT_EQ(results->size(), 1u);
  EXPECT_THAT((*results)[0].references,
              ElementsAre(scada::ReferenceDescription{
                  .reference_type_id = NumericNode(10),
                  .forward = true,
                  .node_id = NumericNode(11)}));
}

TEST_F(OpcUaEndpointCoreTest,
       TranslateBrowsePaths_ForwardsInputsAndResults) {
  const auto inputs = std::vector<scada::BrowsePath>{
      {.node_id = NumericNode(12),
       .relative_path = {{.reference_type_id = NumericNode(13),
                          .inverse = true,
                          .include_subtypes = false,
                          .target_name = {"Leaf", 7}}}}};

  EXPECT_CALL(view_service_, TranslateBrowsePaths(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::BrowsePath>& actual_inputs,
                           const scada::TranslateBrowsePathsCallback& callback) {
        EXPECT_THAT(actual_inputs, ElementsAre(inputs[0]));
        callback(scada::StatusCode::Good,
                 {scada::BrowsePathResult{
                     .status_code = scada::StatusCode::Good,
                     .targets = {{.target_id = scada::ExpandedNodeId{NumericNode(14)},
                                  .remaining_path_index = 0}}}});
      }));

  std::optional<scada::Status> status;
  std::optional<std::vector<scada::BrowsePathResult>> results;
  TranslateBrowsePaths(
      view_service_, inputs,
      [&](scada::Status actual_status,
          std::vector<scada::BrowsePathResult> actual_results) {
        status = std::move(actual_status);
        results = std::move(actual_results);
      });

  ASSERT_TRUE(status.has_value());
  ASSERT_TRUE(results.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Good);
  ASSERT_EQ(results->size(), 1u);
  ASSERT_EQ((*results)[0].targets.size(), 1u);
  EXPECT_EQ((*results)[0].targets[0].target_id,
            scada::ExpandedNodeId{NumericNode(14)});
}

TEST_F(OpcUaEndpointCoreTest, AddNodes_ForwardsInputsAndResults) {
  const auto inputs = std::vector<scada::AddNodesItem>{
      {.requested_id = NumericNode(20),
       .parent_id = NumericNode(21),
       .node_class = scada::NodeClass::Variable,
       .type_definition_id = NumericNode(22),
       .attributes = scada::NodeAttributes{}
                         .set_display_name({u"Flow"})
                         .set_data_type(NumericNode(23))}};

  EXPECT_CALL(node_management_service_, AddNodes(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::AddNodesItem>& actual_inputs,
                           const scada::AddNodesCallback& callback) {
        EXPECT_THAT(actual_inputs, ElementsAre(inputs[0]));
        callback(scada::StatusCode::Good,
                 {scada::AddNodesResult{
                     .status_code = scada::StatusCode::Good,
                     .added_node_id = NumericNode(24)}});
      }));

  std::optional<scada::Status> status;
  std::optional<std::vector<scada::AddNodesResult>> results;
  AddNodes(node_management_service_, inputs,
           [&](scada::Status actual_status,
               std::vector<scada::AddNodesResult> actual_results) {
             status = std::move(actual_status);
             results = std::move(actual_results);
           });

  ASSERT_TRUE(status.has_value());
  ASSERT_TRUE(results.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Good);
  ASSERT_EQ(results->size(), 1u);
  EXPECT_EQ((*results)[0].status_code, scada::StatusCode::Good);
  EXPECT_EQ((*results)[0].added_node_id, NumericNode(24));
}

TEST_F(OpcUaEndpointCoreTest, DeleteNodes_ForwardsInputsAndResults) {
  const auto inputs = std::vector<scada::DeleteNodesItem>{
      {.node_id = NumericNode(30), .delete_target_references = true}};

  EXPECT_CALL(node_management_service_, DeleteNodes(_, _))
      .WillOnce(Invoke(
          [&](const std::vector<scada::DeleteNodesItem>& actual_inputs,
              const scada::DeleteNodesCallback& callback) {
            ASSERT_EQ(actual_inputs.size(), 1u);
            EXPECT_EQ(actual_inputs[0].node_id, inputs[0].node_id);
            EXPECT_EQ(actual_inputs[0].delete_target_references,
                      inputs[0].delete_target_references);
            callback(scada::StatusCode::Good,
                     {scada::StatusCode::Bad_WrongNodeId});
          }));

  std::optional<scada::Status> status;
  std::optional<std::vector<scada::StatusCode>> results;
  DeleteNodes(node_management_service_, inputs,
              [&](scada::Status actual_status,
                  std::vector<scada::StatusCode> actual_results) {
                status = std::move(actual_status);
                results = std::move(actual_results);
              });

  ASSERT_TRUE(status.has_value());
  ASSERT_TRUE(results.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Good);
  EXPECT_THAT(*results, ElementsAre(scada::StatusCode::Bad_WrongNodeId));
}

TEST_F(OpcUaEndpointCoreTest, CallMethod_ForwardsMethodArgumentsAndUserId) {
  const auto node_id = NumericNode(40, 8);
  const auto method_id = NumericNode(41, 8);
  const auto user_id = NumericNode(42, 8);
  const auto arguments =
      std::vector<scada::Variant>{scada::Variant{true},
                                  scada::Variant{scada::Int32{7}}};

  EXPECT_CALL(method_service_,
              Call(node_id, method_id, arguments, user_id, _))
      .WillOnce(Invoke(
          [](const scada::NodeId&,
             const scada::NodeId&,
             const std::vector<scada::Variant>&,
             const scada::NodeId&,
             const scada::StatusCallback& callback) {
            callback(scada::StatusCode::Bad_WrongCallArguments);
          }));

  std::optional<scada::Status> status;
  CallMethod(method_service_, node_id, method_id, arguments, user_id,
             [&](scada::Status actual_status) {
               status = std::move(actual_status);
             });

  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Bad_WrongCallArguments);
}

TEST_F(OpcUaEndpointCoreTest, CreateMonitoredItem_ForwardsInputsAndReturnsItem) {
  const auto read_value_id = scada::ReadValueId{
      .node_id = NumericNode(50, 9),
      .attribute_id = scada::AttributeId::Value};
  scada::MonitoringParameters parameters;
  parameters.sampling_interval = base::TimeDelta::FromMilliseconds(250);
  parameters.queue_size = 3u;
  auto monitored_item =
      std::make_shared<StrictMock<scada::MockMonitoredItem>>();

  EXPECT_CALL(monitored_item_service_, CreateMonitoredItem(_, _))
      .WillOnce(Invoke([&](const scada::ReadValueId& actual_read_value_id,
                           const scada::MonitoringParameters& actual_parameters)
                            -> std::shared_ptr<scada::MonitoredItem> {
        EXPECT_EQ(actual_read_value_id, read_value_id);
        EXPECT_TRUE(actual_parameters.sampling_interval.has_value());
        EXPECT_TRUE(actual_parameters.queue_size.has_value());
        if (actual_parameters.sampling_interval.has_value()) {
          EXPECT_EQ(*actual_parameters.sampling_interval,
                    base::TimeDelta::FromMilliseconds(250));
        }
        if (actual_parameters.queue_size.has_value()) {
          EXPECT_EQ(*actual_parameters.queue_size, 3u);
        }
        return monitored_item;
      }));

  auto created =
      CreateMonitoredItem(monitored_item_service_, read_value_id, parameters);

  EXPECT_EQ(created.status, scada::StatusCode::Good);
  EXPECT_EQ(created.monitored_item, monitored_item);
}

TEST_F(OpcUaEndpointCoreTest,
       CreateMonitoredItem_TranslatesBadAttributeAndUnknownNodeFailures) {
  const auto bad_attribute = scada::ReadValueId{
      .node_id = NumericNode(60, 9),
      .attribute_id = scada::AttributeId::DisplayName};
  const auto unknown_node = scada::ReadValueId{
      .node_id = NumericNode(61, 9),
      .attribute_id = scada::AttributeId::EventNotifier};
  const scada::MonitoringParameters parameters;

  EXPECT_CALL(monitored_item_service_, CreateMonitoredItem(_, _))
      .WillOnce(Invoke([&](const scada::ReadValueId& actual_read_value_id,
                           const scada::MonitoringParameters&) {
        EXPECT_EQ(actual_read_value_id, unknown_node);
        return std::shared_ptr<scada::MonitoredItem>{};
      }));

  auto bad_attribute_result =
      CreateMonitoredItem(monitored_item_service_, bad_attribute, parameters);
  auto unknown_node_result =
      CreateMonitoredItem(monitored_item_service_, unknown_node, parameters);

  EXPECT_EQ(bad_attribute_result.status,
            scada::StatusCode::Bad_WrongAttributeId);
  EXPECT_EQ(unknown_node_result.status, scada::StatusCode::Bad_WrongNodeId);
  EXPECT_FALSE(bad_attribute_result.monitored_item);
  EXPECT_FALSE(unknown_node_result.monitored_item);
}

TEST_F(OpcUaEndpointCoreTest,
       SubscribeMonitoredItemNotifications_UsesDataChangeHandlerForValueItems) {
  auto monitored_item = std::make_shared<scada::TestMonitoredItem>();
  const auto read_value_id = scada::ReadValueId{
      .node_id = NumericNode(70, 9),
      .attribute_id = scada::AttributeId::Value};
  std::optional<scada::DataValue> delivered;

  SubscribeMonitoredItemNotifications(
      read_value_id, monitored_item,
      [&](const scada::DataValue& value) { delivered = value; },
      [&](const scada::Status&, const std::any&) { FAIL(); });

  const scada::DataValue value{scada::Variant{scada::Int32{17}}, {},
                               scada::DateTime::Now(), scada::DateTime::Now()};
  monitored_item->NotifyDataChange(value);

  ASSERT_TRUE(delivered.has_value());
  EXPECT_EQ(*delivered, value);
}

TEST_F(OpcUaEndpointCoreTest,
       SubscribeMonitoredItemNotifications_UsesEventHandlerForEventItems) {
  auto monitored_item = std::make_shared<scada::TestMonitoredItem>();
  const auto read_value_id = scada::ReadValueId{
      .node_id = NumericNode(71, 9),
      .attribute_id = scada::AttributeId::EventNotifier};
  std::optional<scada::Status> delivered_status;
  std::optional<int> delivered_event_id;

  SubscribeMonitoredItemNotifications(
      read_value_id, monitored_item,
      [&](const scada::DataValue&) { FAIL(); },
      [&](const scada::Status& status, const std::any& event) {
        delivered_status = status;
        delivered_event_id = std::any_cast<int>(event);
      });

  monitored_item->NotifyEvent(42);

  ASSERT_TRUE(delivered_status.has_value());
  ASSERT_TRUE(delivered_event_id.has_value());
  EXPECT_EQ(delivered_status->code(), scada::StatusCode::Good);
  EXPECT_EQ(*delivered_event_id, 42);
}

TEST_F(OpcUaEndpointCoreTest,
       DispatchMonitoredItemNotifications_ForwardsMatchingHandlerKinds) {
  const auto value_item = scada::ReadValueId{
      .node_id = NumericNode(72, 9),
      .attribute_id = scada::AttributeId::Value};
  const auto event_item = scada::ReadValueId{
      .node_id = NumericNode(73, 9),
      .attribute_id = scada::AttributeId::EventNotifier};
  const scada::DataValue value{scada::Variant{scada::Int32{23}}, {},
                               scada::DateTime::Now(), scada::DateTime::Now()};
  std::optional<scada::DataValue> delivered_value;
  std::optional<scada::Status> delivered_status;
  std::optional<int> delivered_event_id;

  const std::optional<scada::MonitoredItemHandler> data_handler =
      scada::DataChangeHandler{
          [&](const scada::DataValue& data_value) { delivered_value = data_value; }};
  const std::optional<scada::MonitoredItemHandler> event_handler =
      scada::EventHandler{[&](const scada::Status& status, const std::any& event) {
        delivered_status = status;
        delivered_event_id = std::any_cast<int>(event);
      }};

  EXPECT_TRUE(
      DispatchDataChangeNotification(value_item, data_handler, value));
  EXPECT_TRUE(DispatchEventNotification(event_item, event_handler,
                                        scada::StatusCode::Good, 91));
  EXPECT_FALSE(
      DispatchDataChangeNotification(event_item, event_handler, value));
  EXPECT_FALSE(DispatchEventNotification(value_item, data_handler,
                                         scada::StatusCode::Good, 91));

  ASSERT_TRUE(delivered_value.has_value());
  EXPECT_EQ(*delivered_value, value);
  ASSERT_TRUE(delivered_status.has_value());
  EXPECT_EQ(delivered_status->code(), scada::StatusCode::Good);
  ASSERT_TRUE(delivered_event_id.has_value());
  EXPECT_EQ(*delivered_event_id, 91);
}

TEST_F(OpcUaEndpointCoreTest,
       ProjectEventFields_PreservesRequestedSelectClauseOrder) {
  scada::Event event;
  event.event_id = 11;
  event.event_type_id = NumericNode(501, 0);
  event.node_id = NumericNode(777, 4);
  event.time = scada::DateTime::Now();
  event.message = scada::LocalizedText{u"alarm"};
  event.severity = 900;

  const auto fields = ProjectEventFields(
      {{"Severity"}, {"Message"}, {"EventId"}, {"UnknownField"}}, event);

  ASSERT_EQ(fields.size(), 4u);
  EXPECT_EQ(fields[0].get<scada::UInt32>(), 900u);
  EXPECT_EQ(fields[1].get<scada::LocalizedText>(),
            scada::LocalizedText{u"alarm"});
  EXPECT_EQ(fields[2].get<scada::UInt64>(), 11u);
  EXPECT_TRUE(fields[3].is_null());
}

TEST_F(OpcUaEndpointCoreTest,
       NormalizeEventFieldPaths_UsesDefaultsOnlyWhenEmpty) {
  const auto defaults = NormalizeEventFieldPaths({});
  EXPECT_EQ(defaults, DefaultEventFieldPaths());

  const std::vector<std::vector<std::string>> custom{
      {"Severity"},
      {"Message"},
  };
  EXPECT_EQ(NormalizeEventFieldPaths(custom), custom);
}

TEST_F(OpcUaEndpointCoreTest, ParseAndBuildEventFilter_RoundTripsFieldPaths) {
  const auto filter = BuildEventFilter(std::vector<std::vector<std::string>>{
      {"Severity"},
      {"Message"},
  });
  EXPECT_EQ(ParseEventFilterFieldPaths(filter),
            (std::vector<std::vector<std::string>>{{"Severity"},
                                                   {"Message"}}));
}

TEST_F(OpcUaEndpointCoreTest,
       DispatchEventFieldNotification_AssemblesAndForwardsModelChangeEvents) {
  const auto event_item = scada::ReadValueId{
      .node_id = NumericNode(74, 9),
      .attribute_id = scada::AttributeId::EventNotifier};
  std::optional<scada::Status> delivered_status;
  std::optional<scada::ModelChangeEvent> delivered_event;

  const std::optional<scada::MonitoredItemHandler> event_handler =
      scada::EventHandler{[&](const scada::Status& status, const std::any& event) {
        delivered_status = status;
        delivered_event = std::any_cast<scada::ModelChangeEvent>(event);
      }};

  const auto fields = std::vector<scada::Variant>{
      scada::NodeId{scada::id::GeneralModelChangeEventType},
      NumericNode(500, 2),
      NumericNode(600, 3),
      static_cast<scada::UInt32>(
          scada::ModelChangeEvent::NodeAdded |
          scada::ModelChangeEvent::ReferenceAdded),
  };

  EXPECT_TRUE(DispatchEventFieldNotification(event_item, event_handler, fields));

  ASSERT_TRUE(delivered_status.has_value());
  ASSERT_TRUE(delivered_event.has_value());
  EXPECT_EQ(delivered_status->code(), scada::StatusCode::Good);
  EXPECT_EQ(delivered_event->node_id, NumericNode(500, 2));
  EXPECT_EQ(delivered_event->type_definition_id, NumericNode(600, 3));
  EXPECT_EQ(delivered_event->verb,
            static_cast<uint8_t>(scada::ModelChangeEvent::NodeAdded |
                                 scada::ModelChangeEvent::ReferenceAdded));
}

}  // namespace
}  // namespace scada::opcua_endpoint
