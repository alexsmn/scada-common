#pragma once

#include "scada/monitored_item.h"

class VidiconMonitoredEvents : public scada::MonitoredItem {
 public:
  virtual void Subscribe(scada::MonitoredItemHandler handler) override {}
};
