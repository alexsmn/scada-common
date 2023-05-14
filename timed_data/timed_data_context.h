#pragma once

#include "common/aliases.h"
#include "core/node_id.h"

#include <memory>

namespace scada {
class AttributeService;
class AddressSpace;
class HistoryService;
class MethodService;
class MonitoredItemService;
}  // namespace scada

class Executor;
class NodeEventProvider;
class NodeService;

struct TimedDataContext {
  const std::shared_ptr<Executor> executor_;
  const AliasResolver alias_resolver_;
  NodeService& node_service_;
  scada::AttributeService& attribute_service_;
  scada::MethodService& method_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::HistoryService& history_service_;
  NodeEventProvider& node_event_provider_;
};