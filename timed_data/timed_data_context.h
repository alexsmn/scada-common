#pragma once

#include <boost/asio/io_service.hpp>

namespace events {
class EventManager;
}

namespace scada {
class AttributeService;
class MethodService;
class MonitoredItemService;
class EventService;
class HistoryService;
}

class NodeService;

struct TimedDataContext {
  boost::asio::io_service& io_service_;
  NodeService& node_service_;
  scada::MonitoredItemService& realtime_service_;
  scada::AttributeService& attribute_service_;
  scada::MethodService& method_service_;
  scada::EventService& event_service_;
  scada::HistoryService& history_service_;
  events::EventManager& event_manager_;
};