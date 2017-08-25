#pragma once

#include <memory>

#include "server/core/variable_handle.h"

class PropertyVariable : public scada::VariableHandle {
 public:
  PropertyVariable(std::shared_ptr<scada::VariableHandle> base_variable,
                   const scada::NodeId& prop_type_id);

  void Init();

 private:
  std::unique_ptr<scada::MonitoredItem> monitored_item_;
  scada::NodeId prop_type_id_;
};