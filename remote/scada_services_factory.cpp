#include "core/data_services_factory.h"

#include "base/strings/string_util.h"
#include "remote/event_service_proxy.h"
#include "remote/session_proxy.h"
#include "remote/view_service_proxy.h"

bool CreateScadaServices(const DataServicesContext& context, DataServices& services) {
  auto session_proxy = std::make_shared<SessionProxy>(context.logger, context.transport_factory);
  services = {
      session_proxy,
      std::shared_ptr<scada::ViewService>(session_proxy, &session_proxy->GetViewService()),
      std::shared_ptr<scada::NodeManagementService>(session_proxy, &session_proxy->GetNodeManagementService()),
      std::shared_ptr<scada::EventService>(session_proxy, &session_proxy->GetEventService()),
      std::shared_ptr<scada::HistoryService>(session_proxy, &session_proxy->GetHistoryService()),
      session_proxy,
      session_proxy,
      session_proxy,
  };
  return true;
}
