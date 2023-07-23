#include "scada/data_services_factory.h"

#include "base/strings/string_util.h"
#include "vidicon/vidicon_session.h"

#include "TeleClient_i.c"

bool CreateVidiconServices(const DataServicesContext& context,
                           DataServices& services) {
  auto vidicon_session = std::make_shared<VidiconSession>();
  services = {.session_service_ = vidicon_session,
              .view_service_ = vidicon_session,
              .node_management_service_ = vidicon_session,
              .history_service_ = vidicon_session,
              .attribute_service_ = vidicon_session,
              .method_service_ = vidicon_session,
              .monitored_item_service_ = vidicon_session};
  return true;
}
