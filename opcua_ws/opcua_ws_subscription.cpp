#include "opcua_ws/opcua_ws_subscription.h"

#include "opcua/opcua_endpoint_core.h"

#include "scada/event.h"
#include "scada/monitored_item.h"
#include "scada/monitoring_parameters.h"

#include <algorithm>
#include <variant>

namespace opcua {

OpcUaSubscription::OpcUaSubscription(
    OpcUaSubscriptionId subscription_id,
    OpcUaSubscriptionParameters parameters,
    scada::MonitoredItemService& monitored_item_service,
    base::Time publish_cycle_start_time)
    : subscription_id_{subscription_id},
      parameters_{std::move(parameters)},
      monitored_item_service_{monitored_item_service},
      last_publish_time_{publish_cycle_start_time} {}

OpcUaModifySubscriptionResponse OpcUaSubscription::Modify(
    const OpcUaModifySubscriptionRequest& request) {
  if (request.subscription_id != subscription_id_) {
    return {.status = scada::StatusCode::Bad_WrongSubscriptionId};
  }

  parameters_ = request.parameters;
  return {.status = scada::StatusCode::Good,
          .revised_publishing_interval_ms =
              parameters_.publishing_interval_ms,
          .revised_lifetime_count = parameters_.lifetime_count,
          .revised_max_keep_alive_count = parameters_.max_keep_alive_count};
}

void OpcUaSubscription::SetPublishingEnabled(bool publishing_enabled) {
  parameters_.publishing_enabled = publishing_enabled;
}

bool OpcUaSubscription::IsPublishReady(base::Time now) const {
  if (!last_publish_time_.has_value())
    return false;

  const auto elapsed = now - *last_publish_time_;
  if (initial_message_sent_ && parameters_.publishing_enabled &&
      !pending_notifications_.empty()) {
    return elapsed >= PublishingInterval();
  }

  if (!initial_message_sent_) {
    return elapsed >= PublishingInterval();
  }

  if (!parameters_.publishing_enabled || pending_notifications_.empty()) {
    return elapsed >= KeepAliveInterval();
  }

  if (!pending_notifications_.empty()) {
    return now - *last_publish_time_ >= PublishingInterval();
  }
  return elapsed >= KeepAliveInterval();
}

void OpcUaSubscription::PrimePublishCycle(base::Time now) {
  if (!parameters_.publishing_enabled || last_publish_time_.has_value())
    return;
  last_publish_time_ = now;
}

std::optional<base::Time> OpcUaSubscription::NextPublishDeadline() const {
  if (!last_publish_time_.has_value())
    return std::nullopt;

  if (!initial_message_sent_) {
    return *last_publish_time_ + PublishingInterval();
  }

  return *last_publish_time_ +
         ((parameters_.publishing_enabled && !pending_notifications_.empty())
              ? PublishingInterval()
              : KeepAliveInterval());
}

OpcUaCreateMonitoredItemsResponse OpcUaSubscription::CreateMonitoredItems(
    const OpcUaCreateMonitoredItemsRequest& request) {
  if (request.subscription_id != subscription_id_) {
    return {.status = scada::StatusCode::Bad_WrongSubscriptionId};
  }

  OpcUaCreateMonitoredItemsResponse response{
      .status = scada::StatusCode::Good};
  response.results.reserve(request.items_to_create.size());

  for (const auto& source_item : request.items_to_create) {
    auto item = std::make_shared<Item>();
    item->monitored_item_id = next_monitored_item_id_++;
    item->item_to_monitor = source_item.item_to_monitor;
    item->index_range = source_item.index_range;
    item->monitoring_mode = source_item.monitoring_mode;
    item->parameters = source_item.requested_parameters;
    item->event_field_paths =
        ParseEventFieldPaths(source_item.requested_parameters.filter);

    items_.emplace(item->monitored_item_id, item);
    RebindItem(*item);

    response.results.push_back(
        {.status = item->monitored_item_status,
         .monitored_item_id = item->monitored_item ? item->monitored_item_id : 0,
         .revised_sampling_interval_ms = item->parameters.sampling_interval_ms,
         .revised_queue_size = std::max<scada::UInt32>(1, item->parameters.queue_size)});
    if (!item->monitored_item)
      items_.erase(item->monitored_item_id);
  }

  return response;
}

OpcUaModifyMonitoredItemsResponse OpcUaSubscription::ModifyMonitoredItems(
    const OpcUaModifyMonitoredItemsRequest& request) {
  if (request.subscription_id != subscription_id_) {
    return {.status = scada::StatusCode::Bad_WrongSubscriptionId};
  }

  OpcUaModifyMonitoredItemsResponse response{
      .status = scada::StatusCode::Good};
  response.results.reserve(request.items_to_modify.size());

  for (const auto& source_item : request.items_to_modify) {
    const auto item_it = items_.find(source_item.monitored_item_id);
    if (item_it == items_.end()) {
      // OPC UA Part 4 v1.04, 5.12.3.4 Table 74 defines
      // Bad_MonitoredItemIdInvalid as the operation-level result when the
      // monitoredItemId is unknown.
      // URL: https://reference.opcfoundation.org/Core/Part4/v104/docs/5.12.3
      response.results.push_back(
          {.status = scada::StatusCode::Bad_MonitoredItemIdInvalid});
      continue;
    }

    auto& item = *item_it->second;
    item.parameters = source_item.requested_parameters;
    item.event_field_paths =
        ParseEventFieldPaths(source_item.requested_parameters.filter);
    RebindItem(item);

    response.results.push_back(
        {.status = item.monitored_item_status,
         .revised_sampling_interval_ms = item.parameters.sampling_interval_ms,
         .revised_queue_size = std::max<scada::UInt32>(1, item.parameters.queue_size)});
  }

  return response;
}

OpcUaDeleteMonitoredItemsResponse OpcUaSubscription::DeleteMonitoredItems(
    const OpcUaDeleteMonitoredItemsRequest& request) {
  if (request.subscription_id != subscription_id_) {
    return {.status = scada::StatusCode::Bad_WrongSubscriptionId};
  }

  OpcUaDeleteMonitoredItemsResponse response{
      .status = scada::StatusCode::Good};
  response.results.reserve(request.monitored_item_ids.size());

  for (auto monitored_item_id : request.monitored_item_ids) {
    const auto erased = items_.erase(monitored_item_id);
    // OPC UA Part 4 v1.04, 5.12.6.4 Table 83 defines
    // Bad_MonitoredItemIdInvalid as the operation-level result when the
    // monitoredItemId is unknown.
    // URL: https://reference.opcfoundation.org/Core/Part4/v104/docs/5.12.6
    response.results.push_back(
        erased ? scada::StatusCode::Good
               : scada::StatusCode::Bad_MonitoredItemIdInvalid);
  }

  pending_notifications_.erase(
      std::remove_if(
          pending_notifications_.begin(),
          pending_notifications_.end(),
          [&](const auto& queued) {
            return std::find(request.monitored_item_ids.begin(),
                             request.monitored_item_ids.end(),
                             queued.source_item_id) !=
                   request.monitored_item_ids.end();
          }),
      pending_notifications_.end());

  return response;
}

OpcUaSetMonitoringModeResponse OpcUaSubscription::SetMonitoringMode(
    const OpcUaSetMonitoringModeRequest& request) {
  if (request.subscription_id != subscription_id_) {
    return {.status = scada::StatusCode::Bad_WrongSubscriptionId};
  }

  OpcUaSetMonitoringModeResponse response{
      .status = scada::StatusCode::Good};
  response.results.reserve(request.monitored_item_ids.size());

  for (auto monitored_item_id : request.monitored_item_ids) {
    const auto item_it = items_.find(monitored_item_id);
    if (item_it == items_.end()) {
      // OPC UA Part 4 v1.04, 5.12.4.4 Table 77 defines
      // Bad_MonitoredItemIdInvalid as the operation-level result when the
      // monitoredItemId is unknown.
      // URL: https://reference.opcfoundation.org/Core/Part4/v104/docs/5.12.4
      response.results.push_back(scada::StatusCode::Bad_MonitoredItemIdInvalid);
      continue;
    }

    item_it->second->monitoring_mode = request.monitoring_mode;
    response.results.push_back(scada::StatusCode::Good);
  }

  return response;
}

std::vector<scada::StatusCode> OpcUaSubscription::Acknowledge(
    const std::vector<scada::UInt32>& sequence_numbers) {
  std::vector<scada::StatusCode> results;
  results.reserve(sequence_numbers.size());
  for (const auto sequence_number : sequence_numbers)
    results.push_back(Acknowledge(sequence_number));
  return results;
}

std::optional<OpcUaPublishResponse> OpcUaSubscription::TryPublish(
    base::Time now) {
  PrimePublishCycle(now);
  const bool has_publishable_notifications =
      parameters_.publishing_enabled && !pending_notifications_.empty();
  if (!has_publishable_notifications) {
    if (!IsPublishReady(now))
      return std::nullopt;

    last_publish_time_ = now;
    initial_message_sent_ = true;
    return OpcUaPublishResponse{
        .status = scada::StatusCode::Good,
        .subscription_id = subscription_id_,
        .results = {},
        .more_notifications = false,
        // OPC UA Part 4 sends the first keep-alive at the end of the first
        // publishing cycle and requires all keep-alives to carry the next
        // NotificationMessage sequence number.
        .notification_message = {.sequence_number = next_sequence_number_,
                                 .publish_time = now},
        .available_sequence_numbers = AvailableSequenceNumbers()};
  }

  if (!IsPublishReady(now))
    return std::nullopt;

  auto queued = std::move(pending_notifications_.front());
  pending_notifications_.pop_front();

  OpcUaNotificationMessage notification_message{
      .sequence_number = next_sequence_number_++,
      .publish_time = now,
      .notification_data = {std::move(queued.notification)}};
  retransmit_queue_.push_back(notification_message);
  last_publish_time_ = now;
  initial_message_sent_ = true;

  return OpcUaPublishResponse{
      .status = scada::StatusCode::Good,
      .subscription_id = subscription_id_,
      .results = {},
      .more_notifications = !pending_notifications_.empty(),
      .notification_message = std::move(notification_message),
      .available_sequence_numbers = AvailableSequenceNumbers()};
}

OpcUaRepublishResponse OpcUaSubscription::Republish(
    scada::UInt32 sequence_number) const {
  const auto it = std::find_if(
      retransmit_queue_.begin(), retransmit_queue_.end(),
      [&](const auto& notification_message) {
        return notification_message.sequence_number == sequence_number;
      });
  if (it == retransmit_queue_.end()) {
    // OPC UA Part 4 v1.05, 5.14.6.3 Table 93 defines
    // Bad_MessageNotAvailable when the requested NotificationMessage is no
    // longer in the retransmission queue.
    // URL: https://reference.opcfoundation.org/Core/Part4/v105/docs/5.14
    return {.status = scada::StatusCode::Bad_MessageNotAvailable};
  }
  return {.status = scada::StatusCode::Good, .notification_message = *it};
}

scada::MonitoringParameters OpcUaSubscription::ToMonitoringParameters(
    const Item& item,
    const OpcUaMonitoringParameters& parameters) {
  scada::MonitoringParameters result;
  result.sampling_interval =
      base::TimeDelta::FromMilliseconds(
          static_cast<int64_t>(parameters.sampling_interval_ms));
  result.queue_size = std::max<size_t>(1, parameters.queue_size);

  if (const auto* filter = parameters.filter
                               ? std::get_if<OpcUaDataChangeFilter>(
                                     &*parameters.filter)
                               : nullptr) {
    result.filter = scada::DataChangeFilter{
        .deadband_value = filter->deadband_value};
  } else if (scada::opcua_endpoint::IsAttributeEventNotifier(
                 item.item_to_monitor.attribute_id)) {
    result.filter = scada::EventFilter{};
  }

  return result;
}

scada::StatusCode OpcUaSubscription::Acknowledge(
    scada::UInt32 sequence_number) {
  const auto it = std::find_if(
      retransmit_queue_.begin(), retransmit_queue_.end(),
      [&](const auto& notification_message) {
        return notification_message.sequence_number == sequence_number;
      });
  if (it == retransmit_queue_.end())
    return scada::StatusCode::Bad_MessageNotAvailable;
  retransmit_queue_.erase(it);
  return scada::StatusCode::Good;
}

std::vector<scada::UInt32> OpcUaSubscription::AvailableSequenceNumbers()
    const {
  std::vector<scada::UInt32> result;
  result.reserve(retransmit_queue_.size());
  for (const auto& notification_message : retransmit_queue_)
    result.push_back(notification_message.sequence_number);
  return result;
}

base::TimeDelta OpcUaSubscription::PublishingInterval() const {
  const auto interval_ms =
      static_cast<int64_t>(parameters_.publishing_interval_ms);
  return base::TimeDelta::FromMilliseconds(std::max<int64_t>(1, interval_ms));
}

base::TimeDelta OpcUaSubscription::KeepAliveInterval() const {
  const auto interval_ms = static_cast<int64_t>(PublishingInterval().InMilliseconds()) *
      static_cast<int64_t>(
                           std::max<scada::UInt32>(1,
                                                   parameters_.max_keep_alive_count));
  return base::TimeDelta::FromMilliseconds(interval_ms);
}

bool OpcUaSubscription::IsKeepAliveDue(base::Time now) const {
  if (!last_publish_time_.has_value()) {
    return false;
  }
  return now - *last_publish_time_ >= KeepAliveInterval();
}

void OpcUaSubscription::RebindItem(Item& item) {
  ++item.binding_generation;
  auto created = scada::opcua_endpoint::CreateMonitoredItem(
      monitored_item_service_, item.item_to_monitor,
      ToMonitoringParameters(item, item.parameters));
  item.monitored_item = std::move(created.monitored_item);
  item.monitored_item_status = created.status;
  if (!item.monitored_item)
    return;

  const auto item_it = items_.find(item.monitored_item_id);
  const std::weak_ptr<Item> weak_item =
      item_it != items_.end() ? item_it->second : std::weak_ptr<Item>{};
  const auto binding_generation = item.binding_generation;
  item.monitored_item->Subscribe(scada::opcua_endpoint::MakeMonitoredItemHandler(
      item.item_to_monitor,
      [this, weak_item, binding_generation](const scada::DataValue& data_value) {
        const auto item = weak_item.lock();
        if (!item || item->binding_generation != binding_generation)
          return;
        QueueDataChange(*item, data_value);
      },
      [this, weak_item, binding_generation](const scada::Status& status,
                                            const std::any& event) {
        const auto item = weak_item.lock();
        if (!item || item->binding_generation != binding_generation)
          return;
        QueueEvent(*item, status, event);
      }));
}

void OpcUaSubscription::QueueDataChange(
    Item& item,
    const scada::DataValue& data_value) {
  if (item.monitoring_mode != OpcUaMonitoringMode::Reporting)
    return;
  QueueNotification(
      item,
      OpcUaDataChangeNotification{.monitored_items = {{.client_handle =
                                                           item.parameters.client_handle,
                                                       .value = data_value}}});
}

void OpcUaSubscription::QueueEvent(Item& item,
                                     const scada::Status& status,
                                     const std::any& event) {
  if (!status) {
    QueueNotification(
        item, OpcUaStatusChangeNotification{.status = status.code()});
    return;
  }
  if (item.monitoring_mode != OpcUaMonitoringMode::Reporting)
    return;
  QueueNotification(
      item,
      OpcUaEventNotificationList{
          .events = {{.client_handle = item.parameters.client_handle,
                      .event_fields =
                          BuildEventFields(item.event_field_paths, event)}}});
}

void OpcUaSubscription::QueueNotification(
    Item& item,
    OpcUaNotificationData notification) {
  pending_notifications_.push_back(
      {.source_item_id = item.monitored_item_id,
       .notification = std::move(notification)});
  EnforceQueueLimit(item);
}

void OpcUaSubscription::EnforceQueueLimit(const Item& item) {
  const auto queue_size = std::max<scada::UInt32>(1, item.parameters.queue_size);
  std::vector<size_t> indices;
  for (size_t i = 0; i < pending_notifications_.size(); ++i) {
    if (pending_notifications_[i].source_item_id == item.monitored_item_id)
      indices.push_back(i);
  }
  if (indices.size() <= queue_size)
    return;

  if (!item.parameters.discard_oldest) {
    pending_notifications_.erase(
        pending_notifications_.begin() + static_cast<std::ptrdiff_t>(indices.back()));
    return;
  }

  pending_notifications_.erase(
      pending_notifications_.begin() + static_cast<std::ptrdiff_t>(indices.front()));
}

std::vector<std::vector<std::string>>
OpcUaSubscription::ParseEventFieldPaths(
    const std::optional<OpcUaMonitoringFilter>& filter) {
  const auto* raw_filter = filter ? std::get_if<boost::json::value>(&*filter)
                                  : nullptr;
  if (!raw_filter)
    return scada::opcua_endpoint::DefaultEventFieldPaths();
  return scada::opcua_endpoint::ParseEventFilterFieldPaths(*raw_filter);
}

std::vector<scada::Variant> OpcUaSubscription::BuildEventFields(
    const std::vector<std::vector<std::string>>& field_paths,
    const std::any& event) {
  return scada::opcua_endpoint::ProjectEventFields(field_paths, event);
}

}  // namespace opcua
