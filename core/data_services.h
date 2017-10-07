#pragma once

#include <memory>

namespace scada {
class NodeManagementService;
class EventService;
class HistoryService;
class AttributeService;
class MethodService;
class MonitoredItemService;
class SessionService;
class ViewService;
}

struct DataServices {
  std::shared_ptr<scada::SessionService> session_service_;
  std::shared_ptr<scada::ViewService> view_service_;
  std::shared_ptr<scada::NodeManagementService> node_management_service_;
  std::shared_ptr<scada::EventService> event_service_;
  std::shared_ptr<scada::HistoryService> history_service_;
  std::shared_ptr<scada::AttributeService> attribute_service_;
  std::shared_ptr<scada::MethodService> method_service_;
  std::shared_ptr<scada::MonitoredItemService> monitored_item_service_;
};
