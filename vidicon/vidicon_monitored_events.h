#pragma once

#include "core/monitored_item.h"

class VidiconMonitoredEvents : public scada::MonitoredItem {
 public:
  virtual void Subscribe() override {}
};
