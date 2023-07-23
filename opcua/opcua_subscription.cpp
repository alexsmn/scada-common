#include "opcua_subscription.h"

#include "base/executor.h"
#include "scada/event_util.h"
#include "scada/monitored_item_service.h"
#include "opcua/opcua_conversion.h"

#include <opcuapp/client/session.h>

using namespace std::chrono_literals;

namespace {

const auto kCommitItemsDelay = 1s;

template <typename T>
inline bool Erase(std::vector<T>& vector, const T& item) {
  auto i = std::find(vector.begin(), vector.end(), item);
  if (i == vector.end())
    return false;
  auto p = --vector.end();
  if (i != p)
    *i = *p;
  vector.erase(p);
  return true;
}

template <typename T>
inline bool Contains(const std::vector<T>& vector, const T& item) {
  return std::find(vector.begin(), vector.end(), item) != vector.end();
}

}  // namespace

OpcUaMonitoredItemCreateResult Convert(
    OpcUa_MonitoredItemCreateResult&& source) {
  return {
      source.StatusCode,
      source.MonitoredItemId,
      source.RevisedSamplingInterval,
  };
}

template <typename T, class It>
inline std::vector<T> ConvertVector22(It first, It last) {
  std::vector<T> result(std::distance(first, last));
  std::transform(first, last, result.begin(),
                 [](auto& source) { return Convert(std::move(source)); });
  return result;
}

template <typename T, class Range>
inline std::vector<T> ConvertVector22(Range&& range) {
  return ConvertVector22<T>(std::begin(range), std::end(range));
}

// OpcUaMonitoredItem

class OpcUaMonitoredItem : public scada::MonitoredItem {
 public:
  OpcUaMonitoredItem(std::shared_ptr<OpcUaSubscription> subscription,
                     opcua::MonitoredItemClientHandle client_handle,
                     scada::ReadValueId read_value_id,
                     scada::MonitoringParameters params);
  ~OpcUaMonitoredItem();

  // scada::MonitoredItem
  virtual void Subscribe(scada::MonitoredItemHandler handler) override;

 private:
  const std::shared_ptr<OpcUaSubscription> subscription_;
  const opcua::MonitoredItemClientHandle client_handle_;
  const scada::ReadValueId read_value_id_;
  const scada::MonitoringParameters params_;

  opcua::MonitoredItemId id_ = 0;

  friend class OpcUaSubscription;
};

OpcUaMonitoredItem::OpcUaMonitoredItem(
    std::shared_ptr<OpcUaSubscription> subscription,
    opcua::MonitoredItemClientHandle client_handle,
    scada::ReadValueId read_value_id,
    scada::MonitoringParameters params)
    : subscription_{std::move(subscription)},
      client_handle_{client_handle},
      read_value_id_{std::move(read_value_id)},
      params_{std::move(params)} {
  assert(!read_value_id_.node_id.is_null());
}

OpcUaMonitoredItem::~OpcUaMonitoredItem() {
  subscription_->Unsubscribe(client_handle_);
}

void OpcUaMonitoredItem::Subscribe(scada::MonitoredItemHandler handler) {
  assert(!read_value_id_.node_id.is_null());

  subscription_->Subscribe(client_handle_, read_value_id_, params_,
                           std::move(handler));
}

// OpcUaSubscription

OpcUaSubscription::OpcUaSubscription(OpcUaSubscriptionContext&& context)
    : OpcUaSubscriptionContext{std::move(context)}, subscription_{session_} {}

OpcUaSubscription::~OpcUaSubscription() {
  // TODO: Delete subscription.
}

// static
std::shared_ptr<OpcUaSubscription> OpcUaSubscription::Create(
    OpcUaSubscriptionContext&& context) {
  auto result = std::shared_ptr<OpcUaSubscription>(
      new OpcUaSubscription(std::move(context)));
  result->Init();
  return result;
}

void OpcUaSubscription::Init() {
  CreateSubscription();
}

void OpcUaSubscription::CreateSubscription() {
  opcua::client::SubscriptionParams params{
      500ms,  // publishing_interval
      3000,   // lifetime_count
      10000,  // max_keepalive_count
      0,      // max_notifications_per_publish
      true,   // publishing_enabled
      0,      // priority
  };

  subscription_.Create(
      params,
      BindExecutor(
          executor_, weak_from_this(), [this](opcua::StatusCode status_code) {
            OnCreateSubscriptionResponse(ConvertStatusCode(status_code.code()));
          }));
}

void OpcUaSubscription::OnCreateSubscriptionResponse(scada::Status&& status) {
  if (!status) {
    OnError(std::move(status));
    return;
  }

  created_ = true;

  subscription_.StartPublishing(
      BindExecutor(executor_, weak_from_this(),
                   [this](opcua::StatusCode status_code) {
                     if (!status_code)
                       OnError(ConvertStatusCode(status_code.code()));
                   }),
      BindExecutor(
          executor_, weak_from_this(),
          [this](OpcUa_DataChangeNotification data_change_notification) {
            OnDataChange(std::vector<opcua::MonitoredItemNotification>(
                std::make_move_iterator(
                    data_change_notification.MonitoredItems),
                std::make_move_iterator(
                    data_change_notification.MonitoredItems +
                    data_change_notification.NoOfMonitoredItems)));
          }),
      BindExecutor(
          executor_, weak_from_this(),
          [this](OpcUa_EventNotificationList event_notification_list) {
            OnEvents(std::vector<opcua::EventFieldList>(
                std::make_move_iterator(event_notification_list.Events),
                std::make_move_iterator(event_notification_list.Events +
                                        event_notification_list.NoOfEvents)));
          }));

  CommitItems();
}

std::shared_ptr<scada::MonitoredItem> OpcUaSubscription::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params) {
  assert(!read_value_id.node_id.is_null());
  return std::make_unique<OpcUaMonitoredItem>(
      shared_from_this(), next_monitored_item_client_handle_++, read_value_id,
      params);
}

void OpcUaSubscription::Subscribe(
    opcua::MonitoredItemClientHandle client_handle,
    scada::ReadValueId read_value_id,
    scada::MonitoringParameters params,
    scada::MonitoredItemHandler handler) {
  assert(!read_value_id.node_id.is_null());
  assert(items_.find(client_handle) == items_.end());

  auto& item = items_
                   .emplace(client_handle,
                            Item{
                                client_handle,
                                std::move(read_value_id),
                                std::move(params),
                                std::move(handler),
                                true,
                                false,
                            })
                   .first->second;

  assert(!Contains(pending_subscribe_items_, &item));
  pending_subscribe_items_.emplace_back(&item);

  ScheduleCommitItems();
}

opcua::ExtensionObject ConvertFilter(const scada::MonitoringFilter& filter) {
  /*if (const auto* data_change_filter = std::get_if<) {
      opcua::DataChangeFilter filter;
      filter.DeadbandType = OpcUa_DeadbandType_None;
      filter.Trigger = OpcUa_DataChangeTrigger_StatusValueTimestamp;
      return opcua::ExtensionObject::Encode(std::move(filter));
    }*/

  if (const auto* event_filter = std::get_if<scada::EventFilter>(&filter)) {
    opcua::EventFilter opcua_event_filter;
    Convert(*event_filter, opcua_event_filter);
    return opcua::ExtensionObject::Encode(std::move(opcua_event_filter));
  }

  return opcua::ExtensionObject{};
}

void OpcUaSubscription::CreateMonitoredItems() {
  if (!subscribing_items_.empty())
    return;

  if (pending_subscribe_items_.empty())
    return;

  std::vector<opcua::MonitoredItemCreateRequest> requests(
      pending_subscribe_items_.size());

  for (size_t i = 0; i < requests.size(); ++i) {
    auto& item = *pending_subscribe_items_[i];
    auto& request = requests[i];

    auto opcua_filter = ConvertFilter(item.params.filter);

    request.MonitoringMode = OpcUa_MonitoringMode_Reporting;
    request.RequestedParameters.ClientHandle = item.client_handle;
    Convert(item.read_value_id.node_id, request.ItemToMonitor.NodeId);
    request.ItemToMonitor.AttributeId =
        static_cast<opcua::AttributeId>(item.read_value_id.attribute_id);
    opcua_filter.release(request.RequestedParameters.Filter);
  }

  subscribing_items_.swap(pending_subscribe_items_);

  subscription_.CreateMonitoredItems(
      {requests.data(), requests.size()}, OpcUa_TimestampsToReturn_Both,
      BindExecutor(
          executor_, weak_from_this(),
          [this](opcua::StatusCode status_code,
                 opcua::Span<OpcUa_MonitoredItemCreateResult> results) {
            OnCreateMonitoredItemsResponse(
                ConvertStatusCode(status_code.code()),
                ConvertVector22<OpcUaMonitoredItemCreateResult>(results));
          }));
}

void OpcUaSubscription::OnCreateMonitoredItemsResponse(
    scada::Status&& status,
    std::vector<OpcUaMonitoredItemCreateResult> results) {
  if (!status) {
    OnError(std::move(status));
    return;
  }

  if (results.size() != subscribing_items_.size()) {
    OnError(scada::StatusCode::Bad);
    return;
  }

  auto items = std::move(subscribing_items_);
  subscribing_items_.clear();

  for (size_t i = 0; i < results.size(); ++i) {
    auto& result = results[i];
    auto& item = *items[i];
    assert(!item.added);
    // OutputDebugStringA((std::string{"Subscription "} +
    // item.read_value_id.first.ToString() + " " +
    // std::to_string(static_cast<int>(item.read_value_id.second)) + " result =
    // " +
    //     std::to_string(static_cast<int>(result.status_code.code())) +
    //     "\n").c_str());
    if (result.status_code) {
      item.id = result.monitored_item_id;
      item.added = true;
      if (item.subscribed) {
        // TODO: Forward status.
      } else {
        assert(!Contains(unsubscribing_items_, &item));
        unsubscribing_items_.emplace_back(&item);
        // Commit will be done immediately.
      }

    } else {
      if (item.subscribed) {
        assert(item.handler.has_value());
        // TODO: Forward status.
        if (auto* data_change_handler =
                std::get_if<scada::DataChangeHandler>(&*item.handler)) {
          (*data_change_handler)({ConvertStatusCode(result.status_code.code()),
                                  scada::DateTime::Now()});
        }

      } else {
        assert(items_.find(item.client_handle) != items_.end());
        items_.erase(item.client_handle);
      }
    }
  }

  CommitItems();
}

void OpcUaSubscription::Unsubscribe(
    opcua::MonitoredItemClientHandle client_handle) {
  auto i = items_.find(client_handle);
  assert(i != items_.end());
  auto& item = i->second;

  assert(item.subscribed);
  item.subscribed = false;
  item.handler.reset();

  if (Erase(pending_subscribe_items_, &item)) {
    items_.erase(i);
  } else if (Contains(subscribing_items_, &item)) {
    // Wait for the subscription end.
  } else if (item.added) {
    assert(!Contains(pending_unsubscribe_items_, &item));
    pending_unsubscribe_items_.emplace_back(&item);
    ScheduleCommitItems();
  } else {
    // Add failed.
    items_.erase(i);
  }
}

void OpcUaSubscription::DeleteMonitoredItems() {
  if (!unsubscribing_items_.empty())
    return;

  if (pending_unsubscribe_items_.empty())
    return;

  std::vector<opcua::MonitoredItemId> item_ids(
      pending_unsubscribe_items_.size());
  for (size_t i = 0; i < item_ids.size(); ++i) {
    auto& item = *pending_unsubscribe_items_[i];
    assert(item.added);
    item_ids[i] = item.id;
  }

  unsubscribing_items_.swap(pending_unsubscribe_items_);

  subscription_.DeleteMonitoredItems(
      {item_ids.data(), item_ids.size()},
      BindExecutor(executor_, weak_from_this(),
                   [this](opcua::StatusCode status_code,
                          opcua::Span<OpcUa_StatusCode> results) {
                     std::vector<scada::StatusCode> converted_results(
                         results.size());
                     for (size_t i = 0; i < results.size(); ++i)
                       converted_results[i] = ConvertStatusCode(results[i]);
                     OnDeleteMonitoredItemsResponse(
                         ConvertStatusCode(status_code.code()),
                         std::move(converted_results));
                   }));
}

void OpcUaSubscription::OnDeleteMonitoredItemsResponse(
    scada::Status&& status,
    std::vector<scada::StatusCode> results) {
  if (!status) {
    OnError(std::move(status));
    return;
  }

  if (results.size() != unsubscribing_items_.size()) {
    OnError(scada::StatusCode::Bad);
    return;
  }

  auto items = std::move(unsubscribing_items_);
  unsubscribing_items_.clear();

  for (size_t i = 0; i < results.size(); ++i) {
    auto result = results[i];
    auto& item = *items[i];
    assert(!item.subscribed);
    assert(item.added);
    if (scada::IsGood(result)) {
      assert(items_.find(item.client_handle) != items_.end());
      items_.erase(item.client_handle);
    } else {
      // Unsubscription failed. Unexpected.
      assert(false);
      OnError(result);
      return;
    }
  }

  CommitItems();
}

void OpcUaSubscription::OnError(scada::Status&& status) {
  error_handler_(std::move(status));
}

void OpcUaSubscription::OnDataChange(
    std::vector<opcua::MonitoredItemNotification> notifications) {
  for (auto& notification : notifications) {
    if (auto* item = FindItem(notification.ClientHandle)) {
      if (!item->subscribed)
        continue;
      assert(item->handler.has_value());
      assert(item->read_value_id.attribute_id !=
             scada::AttributeId::EventNotifier);
      if (item->read_value_id.attribute_id !=
          scada::AttributeId::EventNotifier) {
        auto& data_change_handler =
            std::get<scada::DataChangeHandler>(*item->handler);
        auto data_value = Convert(std::move(notification.Value));
        data_change_handler(std::move(data_value));
      }
    }
  }
}

void OpcUaSubscription::OnEvents(
    std::vector<opcua::EventFieldList> notifications) {
  for (auto& notification : notifications) {
    if (auto* item = FindItem(notification.ClientHandle)) {
      if (!item->subscribed)
        continue;
      assert(item->handler.has_value());
      assert(item->read_value_id.attribute_id ==
             scada::AttributeId::EventNotifier);
      if (item->read_value_id.attribute_id ==
          scada::AttributeId::EventNotifier) {
        auto& event_handler = std::get<scada::EventHandler>(*item->handler);
        const auto& fields = ConvertVector<scada::Variant>(
            std::make_move_iterator(notification.EventFields),
            std::make_move_iterator(notification.EventFields +
                                    notification.NoOfEventFields));
        auto event = AssembleEvent(fields);
        if (event.has_value())
          event_handler(scada::StatusCode::Good, std::move(event));
      }
    }
  }
}

OpcUaSubscription::Item* OpcUaSubscription::FindItem(
    opcua::MonitoredItemClientHandle client_handle) {
  auto i = std::find_if(items_.begin(), items_.end(),
                        [client_handle](const auto& p) {
                          return p.second.client_handle == client_handle;
                        });
  return i != items_.end() ? &i->second : nullptr;
}

void OpcUaSubscription::ScheduleCommitItems() {
  if (!created_ || commit_items_scheduled_)
    return;

  commit_items_scheduled_ = true;

  executor_->PostDelayedTask(
      kCommitItemsDelay,
      BindCancelation(weak_from_this(), [this] { ScheduleCommitItemsDone(); }));
}

void OpcUaSubscription::ScheduleCommitItemsDone() {
  commit_items_scheduled_ = false;

  CommitItems();
}

void OpcUaSubscription::CommitItems() {
  assert(created_);

  DeleteMonitoredItems();
  CreateMonitoredItems();
}

void OpcUaSubscription::Reset() {
  subscription_.Reset();
}
