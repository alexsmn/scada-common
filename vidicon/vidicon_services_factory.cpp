#include "core/data_services_factory.h"

#include "base/strings/string_util.h"
#include "vidicon/vidicon_session.h"

#include "TeleClient_i.c"

bool CreateVidiconServices(const DataServicesContext& context, DataServices& services) {
  auto vidicon_session = std::make_shared<VidiconSession>();
  services = {
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
  };
  return true;
}

