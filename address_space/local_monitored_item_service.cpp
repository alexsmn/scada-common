#include "address_space/local_monitored_item_service.h"

#include "base/time/time.h"
#include "common/sync_attribute_service.h"
#include "scada/data_value.h"
#include "scada/item_factory_subscription.h"
#include "scada/monitored_item.h"
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
    const DateTime now = base::Time::Now();
    if (value.source_timestamp.is_null()) {
      value.source_timestamp = now;
    }
    if (value.server_timestamp.is_null()) {
      value.server_timestamp = now;
    }

    (*h)(value);
  }

 private:
  SyncAttributeService& attribute_service_;
  const ReadValueId value_id_;
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

std::shared_ptr<MonitoredItem> LocalMonitoredItemService::CreateItem(
    const ReadValueId& value_id,
    const MonitoringParameters& /*params*/) {
  return std::make_shared<LocalMonitoredItem>(attribute_service_, value_id);
}

}  // namespace scada
