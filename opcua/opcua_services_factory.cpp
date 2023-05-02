#include "core/data_services_factory.h"
#include "opcua/opcua_session.h"

bool CreateOpcUaServices(const DataServicesContext& context,
                         DataServices& services) {
  try {
    auto session = std::make_shared<OpcUaSession>(context.executor);
    services = {.session_service_ = session,
                .view_service_ = session,
                .attribute_service_ = session,
                .method_service_ = session,
                .monitored_item_service_ = session};
    return true;

  } catch (const std::exception&) {
    return false;
  }
}
