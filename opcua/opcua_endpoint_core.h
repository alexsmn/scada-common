#pragma once

#include "scada/attribute_service.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "scada/monitored_item.h"
#include "scada/node_management_service.h"
#include "scada/service_context.h"
#include "scada/view_service.h"

#include <memory>
#include <optional>
#include <utility>

namespace scada::opcua_endpoint {

inline ServiceContext MakeServiceContext(const scada::NodeId& user_id,
                                         ServiceContext base_context = {}) {
  return base_context.with_user_id(user_id);
}

inline scada::DataValue NormalizeReadResult(scada::DataValue result) {
  constexpr unsigned kBadNodeIdUnknownFullCode = 0x80340000u;
  if (result.status_code == scada::StatusCode::Bad_WrongNodeId) {
    result.status_code =
        scada::Status::FromFullCode(kBadNodeIdUnknownFullCode).code();
  }
  return result;
}

inline std::vector<scada::DataValue> NormalizeReadResults(
    std::vector<scada::DataValue> results) {
  for (auto& result : results)
    result = NormalizeReadResult(std::move(result));
  return results;
}

inline void Read(scada::AttributeService& attribute_service,
                 ServiceContext context,
                 std::shared_ptr<const std::vector<scada::ReadValueId>> inputs,
                 const scada::ReadCallback& callback) {
  attribute_service.Read(
      context, std::move(inputs),
      [callback](scada::Status status, std::vector<scada::DataValue> results) {
        callback(std::move(status), NormalizeReadResults(std::move(results)));
      });
}

inline void Write(scada::AttributeService& attribute_service,
                  ServiceContext context,
                  std::shared_ptr<const std::vector<scada::WriteValue>> inputs,
                  const scada::WriteCallback& callback) {
  attribute_service.Write(context, std::move(inputs), callback);
}

inline void Browse(scada::ViewService& view_service,
                   ServiceContext context,
                   std::vector<scada::BrowseDescription> inputs,
                   const scada::BrowseCallback& callback) {
  view_service.Browse(context, std::move(inputs), callback);
}

inline void TranslateBrowsePaths(
    scada::ViewService& view_service,
    std::vector<scada::BrowsePath> inputs,
    const scada::TranslateBrowsePathsCallback& callback) {
  view_service.TranslateBrowsePaths(std::move(inputs), callback);
}

inline void AddNodes(scada::NodeManagementService& node_management_service,
                     std::vector<scada::AddNodesItem> inputs,
                     const scada::AddNodesCallback& callback) {
  node_management_service.AddNodes(std::move(inputs), callback);
}

inline void DeleteNodes(
    scada::NodeManagementService& node_management_service,
    std::vector<scada::DeleteNodesItem> inputs,
    const scada::DeleteNodesCallback& callback) {
  node_management_service.DeleteNodes(std::move(inputs), callback);
}

inline void CallMethod(scada::MethodService& method_service,
                       const scada::NodeId& node_id,
                       const scada::NodeId& method_id,
                       const std::vector<scada::Variant>& arguments,
                       const scada::NodeId& user_id,
                       const scada::StatusCallback& callback) {
  method_service.Call(node_id, method_id, arguments, user_id, callback);
}

inline bool IsAttributeEventNotifier(scada::AttributeId attribute_id) {
  return attribute_id == scada::AttributeId::EventNotifier;
}

inline bool IsSupportedMonitoredAttribute(scada::AttributeId attribute_id) {
  return attribute_id == scada::AttributeId::Value ||
         IsAttributeEventNotifier(attribute_id);
}

inline scada::StatusCode TranslateCreateMonitoredItemFailure(
    const scada::ReadValueId& item_to_monitor) {
  if (!IsSupportedMonitoredAttribute(item_to_monitor.attribute_id)) {
    return scada::StatusCode::Bad_WrongAttributeId;
  }
  return scada::StatusCode::Bad_WrongNodeId;
}

struct CreateMonitoredItemResult {
  std::shared_ptr<scada::MonitoredItem> monitored_item;
  scada::StatusCode status = scada::StatusCode::Bad;
};

inline CreateMonitoredItemResult CreateMonitoredItem(
    scada::MonitoredItemService& monitored_item_service,
    const scada::ReadValueId& item_to_monitor,
    const scada::MonitoringParameters& parameters) {
  if (!IsSupportedMonitoredAttribute(item_to_monitor.attribute_id)) {
    return {.status = scada::StatusCode::Bad_WrongAttributeId};
  }
  auto monitored_item =
      monitored_item_service.CreateMonitoredItem(item_to_monitor, parameters);
  const auto status =
      monitored_item ? scada::StatusCode::Good
                     : TranslateCreateMonitoredItemFailure(item_to_monitor);
  return {.monitored_item = std::move(monitored_item),
          .status = status};
}

template <class DataChangeCallback, class EventCallback>
inline scada::MonitoredItemHandler MakeMonitoredItemHandler(
    const scada::ReadValueId& item_to_monitor,
    DataChangeCallback&& data_change_callback,
    EventCallback&& event_callback) {
  if (IsAttributeEventNotifier(item_to_monitor.attribute_id)) {
    return scada::MonitoredItemHandler{
        scada::EventHandler(std::forward<EventCallback>(event_callback))};
  }
  return scada::MonitoredItemHandler{
      scada::DataChangeHandler(std::forward<DataChangeCallback>(
          data_change_callback))};
}

template <class DataChangeCallback, class EventCallback>
inline void SubscribeMonitoredItemNotifications(
    const scada::ReadValueId& item_to_monitor,
    const std::shared_ptr<scada::MonitoredItem>& monitored_item,
    DataChangeCallback&& data_change_callback,
    EventCallback&& event_callback) {
  if (!monitored_item)
    return;

  monitored_item->Subscribe(MakeMonitoredItemHandler(
      item_to_monitor, std::forward<DataChangeCallback>(data_change_callback),
      std::forward<EventCallback>(event_callback)));
}

inline bool DispatchDataChangeNotification(
    const scada::ReadValueId& item_to_monitor,
    const std::optional<scada::MonitoredItemHandler>& handler,
    const scada::DataValue& data_value) {
  if (IsAttributeEventNotifier(item_to_monitor.attribute_id) || !handler)
    return false;

  const auto* data_change_handler =
      std::get_if<scada::DataChangeHandler>(&*handler);
  if (!data_change_handler)
    return false;

  (*data_change_handler)(data_value);
  return true;
}

inline bool DispatchEventNotification(
    const scada::ReadValueId& item_to_monitor,
    const std::optional<scada::MonitoredItemHandler>& handler,
    const scada::Status& status,
    const std::any& event) {
  if (!IsAttributeEventNotifier(item_to_monitor.attribute_id) || !handler)
    return false;

  const auto* event_handler = std::get_if<scada::EventHandler>(&*handler);
  if (!event_handler)
    return false;

  (*event_handler)(status, event);
  return true;
}

}  // namespace scada::opcua_endpoint
