#pragma once

#include "core/monitored_item_service.h"

namespace scada {

class MockMonitoredItemService : public MonitoredItemService {
 public:
  MOCK_METHOD2(
      CreateMonitoredItem,
      std::unique_ptr<MonitoredItem>(const ReadValueId& read_value_id,
                                     const MonitoringParameters& params));
};

}  // namespace scada
