#include "address_space/local_monitored_item_service.h"

#include "base/time/time.h"
#include "scada/date_time.h"
#include "common/sync_attribute_service.h"
#include "scada/data_value.h"
#include "scada/item_factory_subscription.h"
#include "scada/monitored_item.h"
#include "scada/attribute_ids.h"
#include "scada/monitoring_parameters.h"
#include "scada/read_value_id.h"
#include "scada/service_context.h"

#include <utility>
#include <variant>

namespace scada {

namespace {

class LocalMonitoredItem : public MonitoredItem {
 public:
  LocalMonitoredItem(SyncAttributeService& attribute_service,
                     ReadValueId value_id)
      : attribute_service_{attribute_service}, value_id_{std::move(value_id)} {}

  void Subscribe(MonitoredItemHandler handler) override {
    auto* h = std::get_if<DataChangeHandler>(&handler);
    if (!h) {
      return;
    }

    // Deliver the node's current attribute value from the address space, so
    // the sample matches what Read returns for the same node.
    DataValue value = ::Read(attribute_service_, ServiceContext{}, value_id_);

    // Address-space attributes carry no timestamps; stamp the delivery time so
    // current-value consumers (IsUpdate ordering) treat the sample as fresh.
    const Time now = scada::Now();
    if (scada::IsNull(value.source_timestamp)) {
      value.source_timestamp = now;
    }
    if (scada::IsNull(value.server_timestamp)) {
      value.server_timestamp = now;
    }

    (*h)(value);
  }

 private:
  SyncAttributeService& attribute_service_;
  const ReadValueId value_id_;
};

// Delivers the events a fixture seeded for one node, once, on subscribe.
// Like its data-change sibling it is not a source of continuous updates: it
// replays a fixed set so a capture is stable across runs.
class LocalEventMonitoredItem : public MonitoredItem {
 public:
  // Parentheses, not braces: std::any is constructible from
  // std::vector<std::any>, so brace-init picks the initializer_list
  // constructor and yields a one-element vector holding the whole vector.
  explicit LocalEventMonitoredItem(std::vector<std::any> events)
      : events_(std::move(events)) {}

  void Subscribe(MonitoredItemHandler handler) override {
    auto* h = std::get_if<EventHandler>(&handler);
    if (!h) {
      return;
    }

    for (const std::any& event : events_) {
      (*h)(Status{StatusCode::Good}, event);
    }
  }

 private:
  const std::vector<std::any> events_;
};

}  // namespace

LocalMonitoredItemService::LocalMonitoredItemService(
    SyncAttributeService& attribute_service)
    : attribute_service_{attribute_service} {}

LocalMonitoredItemService::~LocalMonitoredItemService() = default;

StatusOr<std::unique_ptr<MonitoredItemSubscription>>
LocalMonitoredItemService::CreateSubscription(
    ServiceContext /*context*/,
    MonitoredItemSubscriptionOptions options) {
  return MakeItemFactorySubscription(
      [this](const ReadValueId& value_id, const MonitoringParameters& params) {
        return CreateItem(value_id, params);
      },
      options);
}

void LocalMonitoredItemService::AddEvent(const NodeId& source_node_id,
                                         std::any event) {
  events_.push_back({source_node_id, std::move(event)});
}

std::shared_ptr<MonitoredItem> LocalMonitoredItemService::CreateItem(
    const ReadValueId& value_id,
    const MonitoringParameters& params) {
  // The EventNotifier attribute is what distinguishes "notify me about this
  // node's events" from "notify me about its value" — the same test
  // MakeItemFactorySubscription uses to decide which handler to hand the item,
  // so keying on anything else (the filter, say) yields an item whose
  // Subscribe never matches and a subscription that never notifies.
  if (value_id.attribute_id == AttributeId::EventNotifier) {
    std::vector<std::any> events;
    for (const SeededEvent& seeded : events_) {
      if (seeded.source_node_id == value_id.node_id)
        events.push_back(seeded.event);
    }
    return std::make_shared<LocalEventMonitoredItem>(std::move(events));
  }

  return std::make_shared<LocalMonitoredItem>(attribute_service_, value_id);
}

}  // namespace scada
