#pragma once

#include "common/coroutine_service_resolver.h"
#include "common/data_services_util.h"
#include "node_service/node_service_factory.h"
#include "scada/coroutine_services.h"

#include <memory>

namespace node_service::internal {

inline scada::services MakeLegacyNodeServices(
    const NodeServiceContext& context) {
  return {.attribute_service = &context.attribute_service_,
          .monitored_item_service = &context.monitored_item_service_,
          .method_service = &context.method_service_,
          .view_service = &context.view_service_,
          .session_service = &context.session_service_};
}

inline DataServicesNodeServiceContext MakeDataServicesNodeServiceContext(
    const NodeServiceContext& context) {
  return {
      .executor_ = context.executor_,
      .service_context_ = context.service_context_,
      .data_services_ =
          data_services::FromUnownedServices(MakeLegacyNodeServices(context)),
      .scada_client_ = context.scada_client_};
}

struct ResolvedNodeServices {
  ResolvedNodeServices(AnyExecutor executor, DataServices&& data_services)
      : data_services_{std::move(data_services)} {
    monitored_item_service = &scada::service_resolver::RequireSharedService(
        data_services_.monitored_item_service_);

    session_service = &scada::service_resolver::RequireCoroutineService(
        executor, data_services_.coroutine_session_service_,
        data_services_.session_service_, session_service_adapter);
    attribute_service = &scada::service_resolver::RequireCoroutineService(
        executor, data_services_.coroutine_attribute_service_,
        data_services_.attribute_service_, attribute_service_adapter);
    view_service = &scada::service_resolver::RequireCoroutineService(
        executor, data_services_.coroutine_view_service_,
        data_services_.view_service_, view_service_adapter);
  }

  DataServices data_services_;
  std::unique_ptr<scada::CallbackToCoroutineViewServiceAdapter>
      view_service_adapter;
  std::unique_ptr<scada::CallbackToCoroutineAttributeServiceAdapter>
      attribute_service_adapter;
  std::unique_ptr<scada::PromiseToCoroutineSessionServiceAdapter>
      session_service_adapter;
  scada::CoroutineSessionService* session_service = nullptr;
  scada::CoroutineAttributeService* attribute_service = nullptr;
  scada::CoroutineViewService* view_service = nullptr;
  scada::MonitoredItemService* monitored_item_service = nullptr;
};

}  // namespace node_service::internal
