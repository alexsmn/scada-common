#pragma once

#include "core/attribute_ids.h"

#include <memory>

namespace scada {

class NodeId;
class MonitoredItem;
class Status;

class MonitoredItemService {
 public:
  virtual ~MonitoredItemService() {}
  
  virtual std::unique_ptr<MonitoredItem> CreateMonitoredItem(const NodeId& node_id, AttributeId attribute_id) = 0;
};

} // namespace scada