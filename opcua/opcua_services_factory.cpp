#include "core/data_services_factory.h"
#include "opcua/opcua_session.h"

bool CreateOpcUaServices(const DataServicesContext& context,
                         DataServices& services) {
  try {
    auto session = std::make_shared<OpcUaSession>(context.executor);
    services = {
        session, session, nullptr, nullptr, nullptr, session, session, session,
    };
    return true;

  } catch (const std::exception&) {
    return false;
  }
}
