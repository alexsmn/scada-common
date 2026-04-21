#include "opcua_ws/opcua_ws_subscription.h"

#include "scada/event.h"
#include "scada/monitored_item.h"
#include "scada/monitoring_parameters.h"

#include <algorithm>
#include <variant>

namespace opcua_ws {

namespace {

constexpr std::string_view kEventFilterBody = "Body";
constexpr std::string_view kSelectClauses = "SelectClauses";
constexpr std::string_view kBrowsePath = "BrowsePath";
constexpr std::string_view kName = "Name";

const std::vector<std::vector<std::string>>& DefaultEventFieldPaths() {
  static const auto* const kFields =
      new std::vector<std::vector<std::string>>{{"EventId"},
                                                {"EventType"},
                                                {"SourceNode"},
                                                {"SourceName"},
                                                {"Time"},
                                                {"Message"},
                                                {"Severity"}};
  return *kFields;
}

bool IsAttributeEventNotifier(scada::AttributeId attribute_id) {
  return attribute_id == static_cast<scada::AttributeId>(12);
}

bool IsSupportedMonitoredAttribute(scada::AttributeId attribute_id) {
  return attribute_id == scada::AttributeId::Value ||
         IsAttributeEventNotifier(attribute_id);
}

scada::StatusCode TranslateCreateMonitoredItemFailure(
    const scada::ReadValueId& item_to_monitor) {
  if (!IsSupportedMonitoredAttribute(item_to_monitor.attribute_id)) {
    return scada::StatusCode::Bad_WrongAttributeId;
  }
  return scada::StatusCode::Bad_WrongNodeId;
}

std::optional<std::string> ExtractFieldName(
    const std::vector<std::string>& field_path) {
  if (field_path.empty())
    return std::nullopt;
  return field_path.back();
}

}  // namespace

OpcUaWsSubscription::OpcUaWsSubscription(
    OpcUaWsSubscriptionId subscription_id,
    OpcUaWsSubscriptionParameters parameters,
    scada::MonitoredItemService& monitored_item_service)
    : subscription_id_{subscription_id},
      parameters_{std::move(parameters)},
      monitored_item_service_{monitored_item_service} {}

OpcUaWsModifySubscriptionResponse OpcUaWsSubscription::Modify(
    const OpcUaWsModifySubscriptionRequest& request) {
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

void OpcUaWsSubscription::SetPublishingEnabled(bool publishing_enabled) {
  parameters_.publishing_enabled = publishing_enabled;
}

bool OpcUaWsSubscription::IsPublishReady(base::Time now) const {
  if (!parameters_.publishing_enabled || !last_publish_time_.has_value())
    return false;
  if (!pending_notifications_.empty()) {
    return now - *last_publish_time_ >= PublishingInterval();
  }
  return IsKeepAliveDue(now);
}

void OpcUaWsSubscription::PrimePublishCycle(base::Time now) {
  if (!parameters_.publishing_enabled || last_publish_time_.has_value())
    return;
  last_publish_time_ = now;
}

std::optional<base::Time> OpcUaWsSubscription::NextPublishDeadline() const {
  if (!parameters_.publishing_enabled || !last_publish_time_.has_value())
    return std::nullopt;
  return *last_publish_time_ +
         (pending_notifications_.empty() ? KeepAliveInterval()
                                         : PublishingInterval());
}

OpcUaWsCreateMonitoredItemsResponse OpcUaWsSubscription::CreateMonitoredItems(
    const OpcUaWsCreateMonitoredItemsRequest& request) {
  if (request.subscription_id != subscription_id_) {
    return {.status = scada::StatusCode::Bad_WrongSubscriptionId};
  }

  OpcUaWsCreateMonitoredItemsResponse response{
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

    const auto item_status = item->monitored_item
                                 ? scada::StatusCode::Good
                                 : TranslateCreateMonitoredItemFailure(
                                       item->item_to_monitor);

    response.results.push_back(
        {.status = item_status,
         .monitored_item_id = item->monitored_item ? item->monitored_item_id : 0,
         .revised_sampling_interval_ms = item->parameters.sampling_interval_ms,
         .revised_queue_size = std::max<scada::UInt32>(1, item->parameters.queue_size)});
    if (!item->monitored_item)
      items_.erase(item->monitored_item_id);
  }

  return response;
}

OpcUaWsModifyMonitoredItemsResponse OpcUaWsSubscription::ModifyMonitoredItems(
    const OpcUaWsModifyMonitoredItemsRequest& request) {
  if (request.subscription_id != subscription_id_) {
    return {.status = scada::StatusCode::Bad_WrongSubscriptionId};
  }

  OpcUaWsModifyMonitoredItemsResponse response{
      .status = scada::StatusCode::Good};
  response.results.reserve(request.items_to_modify.size());

  for (const auto& source_item : request.items_to_modify) {
    const auto item_it = items_.find(source_item.monitored_item_id);
    if (item_it == items_.end()) {
      response.results.push_back(
          {.status = scada::StatusCode::Bad});
      continue;
    }

    auto& item = *item_it->second;
    item.parameters = source_item.requested_parameters;
    item.event_field_paths =
        ParseEventFieldPaths(source_item.requested_parameters.filter);
    RebindItem(item);

    const auto item_status = item.monitored_item
                                 ? scada::StatusCode::Good
                                 : TranslateCreateMonitoredItemFailure(
                                       item.item_to_monitor);

    response.results.push_back(
        {.status = item_status,
         .revised_sampling_interval_ms = item.parameters.sampling_interval_ms,
         .revised_queue_size = std::max<scada::UInt32>(1, item.parameters.queue_size)});
  }

  return response;
}

OpcUaWsDeleteMonitoredItemsResponse OpcUaWsSubscription::DeleteMonitoredItems(
    const OpcUaWsDeleteMonitoredItemsRequest& request) {
  if (request.subscription_id != subscription_id_) {
    return {.status = scada::StatusCode::Bad_WrongSubscriptionId};
  }

  OpcUaWsDeleteMonitoredItemsResponse response{
      .status = scada::StatusCode::Good};
  response.results.reserve(request.monitored_item_ids.size());

  for (auto monitored_item_id : request.monitored_item_ids) {
    const auto erased = items_.erase(monitored_item_id);
    response.results.push_back(erased ? scada::StatusCode::Good
                                      : scada::StatusCode::Bad);
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

OpcUaWsSetMonitoringModeResponse OpcUaWsSubscription::SetMonitoringMode(
    const OpcUaWsSetMonitoringModeRequest& request) {
  if (request.subscription_id != subscription_id_) {
    return {.status = scada::StatusCode::Bad_WrongSubscriptionId};
  }

  OpcUaWsSetMonitoringModeResponse response{
      .status = scada::StatusCode::Good};
  response.results.reserve(request.monitored_item_ids.size());

  for (auto monitored_item_id : request.monitored_item_ids) {
    const auto item_it = items_.find(monitored_item_id);
    if (item_it == items_.end()) {
      response.results.push_back(scada::StatusCode::Bad);
      continue;
    }

    item_it->second->monitoring_mode = request.monitoring_mode;
    response.results.push_back(scada::StatusCode::Good);
  }

  return response;
}

std::vector<scada::StatusCode> OpcUaWsSubscription::Acknowledge(
    const std::vector<scada::UInt32>& sequence_numbers) {
  std::vector<scada::StatusCode> results;
  results.reserve(sequence_numbers.size());
  for (const auto sequence_number : sequence_numbers)
    results.push_back(Acknowledge(sequence_number));
  return results;
}

std::optional<OpcUaWsPublishResponse> OpcUaWsSubscription::TryPublish(
    base::Time now) {
  if (!parameters_.publishing_enabled)
    return std::nullopt;

  PrimePublishCycle(now);
  if (pending_notifications_.empty()) {
    if (!IsKeepAliveDue(now))
      return std::nullopt;

    last_publish_time_ = now;
    return OpcUaWsPublishResponse{
        .status = scada::StatusCode::Good,
        .subscription_id = subscription_id_,
        .available_sequence_numbers = AvailableSequenceNumbers(),
        .more_notifications = false,
        .notification_message = {.sequence_number = 0, .publish_time = now},
        .results = {}};
  }

  if (!IsPublishReady(now))
    return std::nullopt;

  auto queued = std::move(pending_notifications_.front());
  pending_notifications_.pop_front();

  OpcUaWsNotificationMessage notification_message{
      .sequence_number = next_sequence_number_++,
      .publish_time = now,
      .notification_data = {std::move(queued.notification)}};
  retransmit_queue_.push_back(notification_message);
  last_publish_time_ = now;

  return OpcUaWsPublishResponse{
      .status = scada::StatusCode::Good,
      .subscription_id = subscription_id_,
      .available_sequence_numbers = AvailableSequenceNumbers(),
      .more_notifications = !pending_notifications_.empty(),
      .notification_message = std::move(notification_message),
      .results = {}};
}

OpcUaWsRepublishResponse OpcUaWsSubscription::Republish(
    scada::UInt32 sequence_number) const {
  const auto it = std::find_if(
      retransmit_queue_.begin(), retransmit_queue_.end(),
      [&](const auto& notification_message) {
        return notification_message.sequence_number == sequence_number;
      });
  if (it == retransmit_queue_.end()) {
    return {.status = scada::StatusCode::Bad};
  }
  return {.status = scada::StatusCode::Good, .notification_message = *it};
}

scada::MonitoringParameters OpcUaWsSubscription::ToMonitoringParameters(
    const Item& item,
    const OpcUaWsMonitoringParameters& parameters) {
  scada::MonitoringParameters result;
  result.sampling_interval =
      base::TimeDelta::FromMilliseconds(
          static_cast<int64_t>(parameters.sampling_interval_ms));
  result.queue_size = std::max<size_t>(1, parameters.queue_size);

  if (const auto* filter = parameters.filter
                               ? std::get_if<OpcUaWsDataChangeFilter>(
                                     &*parameters.filter)
                               : nullptr) {
    result.filter = scada::DataChangeFilter{
        .deadband_value = filter->deadband_value};
  } else if (IsAttributeEventNotifier(item.item_to_monitor.attribute_id)) {
    result.filter = scada::EventFilter{};
  }

  return result;
}

scada::StatusCode OpcUaWsSubscription::Acknowledge(
    scada::UInt32 sequence_number) {
  const auto it = std::find_if(
      retransmit_queue_.begin(), retransmit_queue_.end(),
      [&](const auto& notification_message) {
        return notification_message.sequence_number == sequence_number;
      });
  if (it == retransmit_queue_.end())
    return scada::StatusCode::Bad;
  retransmit_queue_.erase(it);
  return scada::StatusCode::Good;
}

std::vector<scada::UInt32> OpcUaWsSubscription::AvailableSequenceNumbers()
    const {
  std::vector<scada::UInt32> result;
  result.reserve(retransmit_queue_.size());
  for (const auto& notification_message : retransmit_queue_)
    result.push_back(notification_message.sequence_number);
  return result;
}

base::TimeDelta OpcUaWsSubscription::PublishingInterval() const {
  const auto interval_ms =
      static_cast<int64_t>(parameters_.publishing_interval_ms);
  return base::TimeDelta::FromMilliseconds(std::max<int64_t>(1, interval_ms));
}

base::TimeDelta OpcUaWsSubscription::KeepAliveInterval() const {
  const auto interval_ms = static_cast<int64_t>(PublishingInterval().InMilliseconds()) *
      static_cast<int64_t>(
                           std::max<scada::UInt32>(1,
                                                   parameters_.max_keep_alive_count));
  return base::TimeDelta::FromMilliseconds(interval_ms);
}

bool OpcUaWsSubscription::IsKeepAliveDue(base::Time now) const {
  if (!last_publish_time_.has_value()) {
    return false;
  }
  return now - *last_publish_time_ >= KeepAliveInterval();
}

void OpcUaWsSubscription::RebindItem(Item& item) {
  ++item.binding_generation;
  item.monitored_item = monitored_item_service_.CreateMonitoredItem(
      item.item_to_monitor, ToMonitoringParameters(item, item.parameters));
  if (!item.monitored_item)
    return;

  const auto item_it = items_.find(item.monitored_item_id);
  const std::weak_ptr<Item> weak_item =
      item_it != items_.end() ? item_it->second : std::weak_ptr<Item>{};
  const auto binding_generation = item.binding_generation;

  if (IsAttributeEventNotifier(item.item_to_monitor.attribute_id)) {
    item.monitored_item->Subscribe(scada::EventHandler{
        [this, weak_item, binding_generation](const scada::Status& status,
                                              const std::any& event) {
          const auto item = weak_item.lock();
          if (!item || item->binding_generation != binding_generation)
            return;
          QueueEvent(*item, status, event);
        }});
  } else {
    item.monitored_item->Subscribe(scada::DataChangeHandler{
        [this, weak_item, binding_generation](
            const scada::DataValue& data_value) {
          const auto item = weak_item.lock();
          if (!item || item->binding_generation != binding_generation)
            return;
          QueueDataChange(*item, data_value);
        }});
  }
}

void OpcUaWsSubscription::QueueDataChange(
    Item& item,
    const scada::DataValue& data_value) {
  if (item.monitoring_mode != OpcUaWsMonitoringMode::Reporting)
    return;
  QueueNotification(
      item,
      OpcUaWsDataChangeNotification{.monitored_items = {{.client_handle =
                                                             item.parameters.client_handle,
                                                         .value = data_value}}});
}

void OpcUaWsSubscription::QueueEvent(Item& item,
                                     const scada::Status& status,
                                     const std::any& event) {
  if (!status) {
    QueueNotification(
        item, OpcUaWsStatusChangeNotification{.status = status.code()});
    return;
  }
  if (item.monitoring_mode != OpcUaWsMonitoringMode::Reporting)
    return;
  QueueNotification(
      item,
      OpcUaWsEventNotificationList{
          .events = {{.client_handle = item.parameters.client_handle,
                      .event_fields =
                          BuildEventFields(item.event_field_paths, event)}}});
}

void OpcUaWsSubscription::QueueNotification(
    Item& item,
    OpcUaWsNotificationData notification) {
  pending_notifications_.push_back(
      {.source_item_id = item.monitored_item_id,
       .notification = std::move(notification)});
  EnforceQueueLimit(item);
}

void OpcUaWsSubscription::EnforceQueueLimit(const Item& item) {
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
OpcUaWsSubscription::ParseEventFieldPaths(
    const std::optional<OpcUaWsMonitoringFilter>& filter) {
  const auto* raw_filter = filter ? std::get_if<boost::json::value>(&*filter)
                                  : nullptr;
  if (!raw_filter || !raw_filter->is_object())
    return DefaultEventFieldPaths();

  const auto* current = &raw_filter->as_object();
  if (const auto* body_field = current->if_contains(kEventFilterBody);
      body_field != nullptr && body_field->is_object()) {
    current = &body_field->as_object();
  }

  const auto* clauses_value = current->if_contains(kSelectClauses);
  if (!clauses_value || !clauses_value->is_array())
    return DefaultEventFieldPaths();

  std::vector<std::vector<std::string>> result;
  for (const auto& clause_value : clauses_value->as_array()) {
    if (!clause_value.is_object())
      continue;
    const auto& clause = clause_value.as_object();
    const auto* browse_path_value = clause.if_contains(kBrowsePath);
    if (!browse_path_value || !browse_path_value->is_array())
      continue;

    std::vector<std::string> path;
    for (const auto& segment_value : browse_path_value->as_array()) {
      if (!segment_value.is_object())
        continue;
      const auto& segment = segment_value.as_object();
      const auto* name_value = segment.if_contains(kName);
      if (!name_value || !name_value->is_string())
        continue;
      path.emplace_back(name_value->as_string().c_str());
    }
    if (!path.empty())
      result.push_back(std::move(path));
  }

  return result.empty() ? DefaultEventFieldPaths() : result;
}

std::vector<scada::Variant> OpcUaWsSubscription::BuildEventFields(
    const std::vector<std::vector<std::string>>& field_paths,
    const std::any& event) {
  const auto* source_event = std::any_cast<scada::Event>(&event);
  std::vector<scada::Variant> result;
  result.reserve(field_paths.size());

  for (const auto& field_path : field_paths) {
    const auto field_name = ExtractFieldName(field_path);
    if (!source_event || !field_name.has_value()) {
      result.emplace_back(scada::Variant{});
      continue;
    }

    if (*field_name == "EventId") {
      result.emplace_back(source_event->event_id);
    } else if (*field_name == "EventType") {
      result.emplace_back(source_event->event_type_id);
    } else if (*field_name == "SourceNode") {
      result.emplace_back(source_event->node_id);
    } else if (*field_name == "SourceName") {
      result.emplace_back(source_event->node_id.is_null()
                              ? std::string{}
                              : source_event->node_id.ToString());
    } else if (*field_name == "Time") {
      result.emplace_back(source_event->time);
    } else if (*field_name == "Message") {
      result.emplace_back(source_event->message);
    } else if (*field_name == "Severity") {
      result.emplace_back(source_event->severity);
    } else {
      result.emplace_back(scada::Variant{});
    }
  }

  return result;
}

}  // namespace opcua_ws
