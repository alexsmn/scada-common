#pragma once

#include "core/attribute_ids.h"

#include <memory>

namespace scada {

class MonitoredItem;
struct ReadValueId;

class MonitoredItemService {
 public:
  virtual ~MonitoredItemService() {}
  
  virtual std::unique_ptr<MonitoredItem> CreateMonitoredItem(const ReadValueId& read_value_id) = 0;
};

} // namespace scada