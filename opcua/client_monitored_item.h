#pragma once

#include "scada/monitored_item.h"
#include "scada/monitoring_parameters.h"
#include "scada/read_value_id.h"

#include <cstdint>
#include <memory>

namespace opcua {

class OpcUaClientSubscription;

// scada::MonitoredItem instance returned to the Qt client from
// OpcUaClientSession::CreateMonitoredItem. Subscribe() binds a handler and
// launches the server-side monitored item through the owning subscription;
// the destructor unsubscribes.
class OpcUaMonitoredItem final : public scada::MonitoredItem {
 public:
  OpcUaMonitoredItem(std::shared_ptr<OpcUaClientSubscription> subscription,
                     std::uint32_t local_id,
                     scada::ReadValueId read_value_id,
                     scada::MonitoringParameters params);
  ~OpcUaMonitoredItem() override;

  void Subscribe(scada::MonitoredItemHandler handler) override;

 private:
  const std::shared_ptr<OpcUaClientSubscription> subscription_;
  const std::uint32_t local_id_;
  const scada::ReadValueId read_value_id_;
  const scada::MonitoringParameters params_;
};

}  // namespace opcua
