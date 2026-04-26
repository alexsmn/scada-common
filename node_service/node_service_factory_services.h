#pragma once

#include "node_service/node_service_factory.h"
#include "scada/coroutine_services.h"

#include <cassert>
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
          DataServices::FromUnownedServices(MakeLegacyNodeServices(context)),
      .scada_client_ = context.scada_client_};
}

struct ResolvedNodeServices {
  ResolvedNodeServices(AnyExecutor executor, DataServices&& data_services)
      : data_services_{std::move(data_services)} {
    monitored_item_service = data_services_.monitored_item_service_.get();

    session_service = data_services_.coroutine_session_service_.get();
    if (!session_service && data_services_.session_service_) {
      if (auto* service = dynamic_cast<scada::CoroutineSessionService*>(
              data_services_.session_service_.get())) {
        session_service = service;
      } else {
        session_service_adapter =
            std::make_unique<scada::PromiseToCoroutineSessionServiceAdapter>(
                executor, *data_services_.session_service_);
        session_service = session_service_adapter.get();
      }
    }

    attribute_service = data_services_.coroutine_attribute_service_.get();
    if (!attribute_service && data_services_.attribute_service_) {
      if (auto* service = dynamic_cast<scada::CoroutineAttributeService*>(
              data_services_.attribute_service_.get())) {
        attribute_service = service;
      } else {
        attribute_service_adapter =
            std::make_unique<scada::CallbackToCoroutineAttributeServiceAdapter>(
                executor, *data_services_.attribute_service_);
        attribute_service = attribute_service_adapter.get();
      }
    }

    view_service = data_services_.coroutine_view_service_.get();
    if (!view_service && data_services_.view_service_) {
      if (auto* service = dynamic_cast<scada::CoroutineViewService*>(
              data_services_.view_service_.get())) {
        view_service = service;
      } else {
        view_service_adapter =
            std::make_unique<scada::CallbackToCoroutineViewServiceAdapter>(
                executor, *data_services_.view_service_);
        view_service = view_service_adapter.get();
      }
    }

    assert(session_service);
    assert(attribute_service);
    assert(view_service);
    assert(monitored_item_service);
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
