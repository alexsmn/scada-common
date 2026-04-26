#include "scada/data_services_factory.h"

#include "vidicon/services/vidicon_session.h"

#include <TeleClient_i.c>

bool CreateVidiconServices(const DataServicesContext& context,
                           DataServices& services) {
  auto vidicon_session = std::make_shared<VidiconSession>();
  services = {.session_service_ = vidicon_session,
              .view_service_ = vidicon_session,
              .node_management_service_ = vidicon_session,
              .history_service_ = vidicon_session,
              .attribute_service_ = vidicon_session,
              .method_service_ = vidicon_session,
              .monitored_item_service_ = vidicon_session,
              .coroutine_session_service_ =
                  std::shared_ptr<scada::CoroutineSessionService>{
                      vidicon_session,
                      &vidicon_session->coroutine_session_service()},
              .coroutine_view_service_ = vidicon_session,
              .coroutine_node_management_service_ = vidicon_session,
              .coroutine_history_service_ = vidicon_session,
              .coroutine_attribute_service_ = vidicon_session,
              .coroutine_method_service_ = vidicon_session};
  return true;
}
