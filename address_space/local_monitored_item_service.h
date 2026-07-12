#pragma once

#include "scada/monitored_item_service.h"

class SyncAttributeService;

namespace scada {

// In-memory MonitoredItemService that creates items which deliver a single
// DataValue on `Subscribe` — the monitored attribute read synchronously from
// the backing SyncAttributeService (i.e. the in-memory address space) — and
// nothing afterwards.
//
// This makes current-value consumers (write dialogs, graph readouts, tables)
// render the fixture's actual Value attributes, so captures are meaningful
// and stable across runs. It is still not a source of continuous updates.
class LocalMonitoredItemService : public MonitoredItemService {
 public:
  explicit LocalMonitoredItemService(SyncAttributeService& attribute_service);
  ~LocalMonitoredItemService() override;

  StatusOr<std::unique_ptr<MonitoredItemSubscription>> CreateSubscription(
      ServiceContext context,
      MonitoredItemSubscriptionOptions options) override;

 private:
  std::shared_ptr<MonitoredItem> CreateItem(const ReadValueId& value_id,
                                            const MonitoringParameters& params);

  SyncAttributeService& attribute_service_;
};

}  // namespace scada
