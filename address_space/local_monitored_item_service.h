#pragma once

#include "scada/monitored_item_service.h"

namespace scada {

// In-memory MonitoredItemService that creates items which deliver a single
// synthetic DataValue on `Subscribe` and nothing afterwards.
//
// Matches the behavior the screenshot generator previously obtained from a
// gmock-driven `MockMonitoredItemService`: enough to get views to render but
// not a source of continuous updates.
class LocalMonitoredItemService : public MonitoredItemService {
 public:
  LocalMonitoredItemService();
  ~LocalMonitoredItemService() override;

  std::shared_ptr<MonitoredItem> CreateMonitoredItem(
      const ReadValueId& value_id,
      const MonitoringParameters& params) override;
};

}  // namespace scada
