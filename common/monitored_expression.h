#pragma once

#include <memory>
#include <vector>

#include "core/monitored_item.h"
#include "server/core/variable_handle.h"

namespace rt {
class ScadaExpression;
}

namespace scada {
class MonitoredItemService;
}

class MonitoredExpression : public scada::VariableHandle {
 public:
  MonitoredExpression(scada::MonitoredItemService& monitored_item_service,
                      std::unique_ptr<rt::ScadaExpression> expression);

  void Init();

 private:
  void CalculateDataValue();

  scada::MonitoredItemService& monitored_item_service_;
  std::unique_ptr<rt::ScadaExpression> expression_;

  std::vector<std::unique_ptr<scada::MonitoredItem>> operands_;
};

std::unique_ptr<scada::MonitoredItem> CreateMonitoredExpression(scada::MonitoredItemService& monitored_item_service,
    const base::StringPiece& formula, scada::NodeId* single_node_id);
