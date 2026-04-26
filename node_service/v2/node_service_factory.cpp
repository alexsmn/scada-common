#include "node_service/v2/node_service_factory.h"

#include "common/coroutine_session_proxy_notifier.h"
#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "node_service/node_service_factory.h"
#include "node_service/v2/node_service_impl.h"
#include "scada/coroutine_services.h"

#include <memory>

namespace v2 {

namespace {

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
        attribute_service_adapter = std::make_unique<
            scada::CallbackToCoroutineAttributeServiceAdapter>(
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

struct NodeServiceHolder {
  explicit NodeServiceHolder(const NodeServiceContext& node_service_context)
      : view_service_adapter{std::make_unique<
            scada::CallbackToCoroutineViewServiceAdapter>(
            node_service_context.executor_, node_service_context.view_service_)},
        attribute_service_adapter{std::make_unique<
            scada::CallbackToCoroutineAttributeServiceAdapter>(
            node_service_context.executor_,
            node_service_context.attribute_service_)},
        session_service_adapter{
            std::make_unique<scada::PromiseToCoroutineSessionServiceAdapter>(
                node_service_context.executor_,
                node_service_context.session_service_)},
        session_service{*session_service_adapter},
        node_service{MakeNodeServiceImplContext(node_service_context)},
        node_service_notifier{node_service, session_service} {}

  explicit NodeServiceHolder(
      const CoroutineNodeServiceContext& node_service_context)
      : session_service{node_service_context.session_service_},
        node_service{MakeNodeServiceImplContext(node_service_context)},
        node_service_notifier{node_service, session_service} {}

  explicit NodeServiceHolder(DataServicesNodeServiceContext&& node_service_context)
      : resolved_services{std::make_unique<ResolvedNodeServices>(
            node_service_context.executor_,
            std::move(node_service_context.data_services_))},
        session_service{*resolved_services->session_service},
        node_service{MakeNodeServiceImplContext(node_service_context)},
        node_service_notifier{node_service, session_service} {}

  NodeServiceImplContext MakeNodeServiceImplContext(
      const NodeServiceContext& node_service_context) {
    auto view_events_provider =
        MakeViewEventsProvider(node_service_context.monitored_item_service_);

    return NodeServiceImplContext{
        node_service_context.executor_,
        *view_service_adapter,
        *attribute_service_adapter,
        node_service_context.monitored_item_service_,
        view_events_provider,
    };
  }

  NodeServiceImplContext MakeNodeServiceImplContext(
      const CoroutineNodeServiceContext& node_service_context) {
    auto view_events_provider =
        MakeViewEventsProvider(node_service_context.monitored_item_service_);

    return NodeServiceImplContext{
        node_service_context.executor_,
        node_service_context.view_service_,
        node_service_context.attribute_service_,
        node_service_context.monitored_item_service_,
        view_events_provider,
    };
  }

  NodeServiceImplContext MakeNodeServiceImplContext(
      const DataServicesNodeServiceContext& node_service_context) {
    auto view_events_provider =
        MakeViewEventsProvider(*resolved_services->monitored_item_service);

    return NodeServiceImplContext{
        node_service_context.executor_,
        *resolved_services->view_service,
        *resolved_services->attribute_service,
        *resolved_services->monitored_item_service,
        view_events_provider,
    };
  }

  std::unique_ptr<ResolvedNodeServices> resolved_services;
  std::unique_ptr<scada::CallbackToCoroutineViewServiceAdapter>
      view_service_adapter;
  std::unique_ptr<scada::CallbackToCoroutineAttributeServiceAdapter>
      attribute_service_adapter;
  std::unique_ptr<scada::PromiseToCoroutineSessionServiceAdapter>
      session_service_adapter;
  scada::CoroutineSessionService& session_service;
  NodeServiceImpl node_service;
  CoroutineSessionProxyNotifier<NodeServiceImpl> node_service_notifier;
};

}  // namespace

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& node_service_context) {
  auto context = std::make_shared<NodeServiceHolder>(node_service_context);
  return std::shared_ptr<NodeService>{context, &context->node_service};
}

std::shared_ptr<NodeService> CreateNodeService(
    const CoroutineNodeServiceContext& node_service_context) {
  auto context = std::make_shared<NodeServiceHolder>(node_service_context);
  return std::shared_ptr<NodeService>{context, &context->node_service};
}

std::shared_ptr<NodeService> CreateNodeService(
    DataServicesNodeServiceContext&& node_service_context) {
  auto context =
      std::make_shared<NodeServiceHolder>(std::move(node_service_context));
  return std::shared_ptr<NodeService>{context, &context->node_service};
}

}  // namespace v2
