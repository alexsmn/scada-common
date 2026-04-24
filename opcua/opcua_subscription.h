#pragma once

#include "base/any_executor.h"
#include "opcua/client/opcua_client_subscription.h"
#include "scada/monitored_item.h"
#include "scada/monitoring_parameters.h"
#include "scada/read_value_id.h"
#include "scada/status.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

class OpcUaSession;

// Qt-client-facing subscription wrapper. Creates a server-side OPC UA
// subscription lazily on first CreateMonitoredItem, then runs a background
// Publish loop so data-change notifications flow to the registered
// handlers.
class OpcUaSubscription
    : public std::enable_shared_from_this<OpcUaSubscription> {
 public:
  static std::shared_ptr<OpcUaSubscription> Create(OpcUaSession& session);

  OpcUaSubscription(const OpcUaSubscription&) = delete;
  OpcUaSubscription& operator=(const OpcUaSubscription&) = delete;
  ~OpcUaSubscription();

  [[nodiscard]] std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id,
      const scada::MonitoringParameters& params);

  // Invoked by OpcUaMonitoredItem during Subscribe() to attach its handler
  // and launch the server-side monitored item.
  void Subscribe(std::uint32_t local_id,
                 const scada::ReadValueId& read_value_id,
                 const scada::MonitoringParameters& params,
                 scada::MonitoredItemHandler handler);

  // Invoked by OpcUaMonitoredItem during destruction; removes the item
  // from the server and drops the handler.
  void Unsubscribe(std::uint32_t local_id);

 private:
  explicit OpcUaSubscription(OpcUaSession& session);

  void EnsureCreated();
  void StartPublishLoop();

  OpcUaSession& session_;
  std::unique_ptr<opcua::OpcUaClientSubscription> impl_;
  bool is_creating_ = false;
  bool publish_loop_running_ = false;
  std::uint32_t next_local_id_ = 1;

  // Maps the OpcUaMonitoredItem's local id to the server-assigned
  // OpcUaMonitoredItemId, set once the create completes.
  std::unordered_map<std::uint32_t, opcua::OpcUaMonitoredItemId>
      server_ids_by_local_id_;
};
