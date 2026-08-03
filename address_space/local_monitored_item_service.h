#pragma once

#include "scada/monitored_item_service.h"
#include "scada/node_id.h"

#include <any>
#include <vector>

class SyncAttributeService;

namespace scada {

// In-memory MonitoredItemService that creates items which deliver a single
// DataValue on `Subscribe` — the monitored attribute read synchronously from
// the backing SyncAttributeService (i.e. the in-memory address space) — and
// nothing afterwards.
//
// This makes current-value consumers (write dialogs, graph readouts, tables)
// render the fixture's actual Value attributes, so captures are meaningful
// and stable across runs. It is still not a source of continuous updates.
//
// An *event* subscription (one monitoring the EventNotifier attribute, the
// same test MakeItemFactorySubscription uses) is served from events seeded
// with AddEvent instead. Without
// this a view that consumes events rather than values — the device log —
// renders an empty grid no matter what the fixture says, because the address
// space has no events in it.
class LocalMonitoredItemService : public MonitoredItemService {
 public:
  explicit LocalMonitoredItemService(SyncAttributeService& attribute_service);
  ~LocalMonitoredItemService() override;

  // Seeds one event delivered to event subscriptions on `source_node_id`.
  // `event` is the payload the consumer any_casts — scada::Event for a plain
  // log line, scada::DeviceFrameEvent for a decoded protocol frame — so a
  // fixture can exercise both shapes of the same stream.
  //
  // Scoped by source node on purpose: two views seeded from one fixture must
  // stay distinguishable, and a device log showing another device's traffic
  // would be a convincing lie.
  void AddEvent(const NodeId& source_node_id, std::any event);

  StatusOr<std::unique_ptr<MonitoredItemSubscription>> CreateSubscription(
      ServiceContext context,
      MonitoredItemSubscriptionOptions options) override;

 private:
  struct SeededEvent {
    NodeId source_node_id;
    std::any event;
  };

  std::shared_ptr<MonitoredItem> CreateItem(const ReadValueId& value_id,
                                            const MonitoringParameters& params);

  SyncAttributeService& attribute_service_;

  std::vector<SeededEvent> events_;
};

}  // namespace scada
