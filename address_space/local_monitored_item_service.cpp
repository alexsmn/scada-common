#include "address_space/local_monitored_item_service.h"

#include "base/time/time.h"
#include "scada/data_value.h"
#include "scada/monitored_item.h"
#include "scada/monitoring_parameters.h"
#include "scada/read_value_id.h"

#include <random>
#include <variant>

namespace scada {

namespace {

class LocalMonitoredItem : public MonitoredItem {
 public:
  void Subscribe(MonitoredItemHandler handler) override {
    if (auto* h = std::get_if<DataChangeHandler>(&handler)) {
      // Deliver a single deterministic random sample. Using a function-local
      // static rng so values are stable per process and vary across items.
      static std::mt19937 rng{99};
      std::uniform_real_distribution<double> dist(10.0, 200.0);
      const auto now = base::Time::Now();
      (*h)(DataValue{Variant{dist(rng)}, {}, now, now});
    }
  }
};

}  // namespace

LocalMonitoredItemService::LocalMonitoredItemService() = default;
LocalMonitoredItemService::~LocalMonitoredItemService() = default;

std::shared_ptr<MonitoredItem> LocalMonitoredItemService::CreateMonitoredItem(
    const ReadValueId& /*value_id*/,
    const MonitoringParameters& /*params*/) {
  return std::make_shared<LocalMonitoredItem>();
}

}  // namespace scada
