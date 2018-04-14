#pragma once

#include "core/node_id.h"
#include "base/strings/string_piece.h"
#include "common/aliases.h"

namespace boost::asio {
class io_context;
}

namespace events {
class EventManager;
}

namespace scada {
class AttributeService;
class AddressSpace;
class EventService;
class HistoryService;
class MethodService;
class MonitoredItemService;
}

class NodeService;

struct TimedDataContext {
  boost::asio::io_context& io_context_;
  const AliasResolver alias_resolver_;
  NodeService& node_service_;
  scada::AttributeService& attribute_service_;
  scada::MethodService& method_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::EventService& event_service_;
  scada::HistoryService& history_service_;
  events::EventManager& event_manager_;
};