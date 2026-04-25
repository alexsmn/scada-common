#include "base/executor_conversions.h"
#include "opcua/client_session.h"
#include "scada/data_services_factory.h"

bool CreateOpcUaServices(const DataServicesContext& context,
                         DataServices& services) {
  try {
    auto session = std::make_shared<opcua::OpcUaClientSession>(
        context.executor, context.transport_factory);
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
