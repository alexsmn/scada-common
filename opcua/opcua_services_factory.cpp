#include "core/data_services_factory.h"
#include "opcua/opcua_session.h"

bool CreateOpcUaServices(const DataServicesContext& context,
                         DataServices& services) {
  try {
    auto session = std::make_shared<OpcUaSession>(context.io_context);
    services = {
        session, session, nullptr, nullptr, nullptr, session, nullptr, session,
    };
    return true;

  } catch (const std::exception&) {
    return false;
  }
}
