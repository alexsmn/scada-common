#include "node_service/v2/node_service_factory.h"

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "common/coroutine_session_proxy_notifier.h"
#include "node_service/node_service_factory.h"
#include "node_service/node_service_factory_services.h"
#include "node_service/v2/node_service_impl.h"

#include <memory>

namespace v2 {

namespace {
using node_service::internal::MakeDataServicesNodeServiceContext;
using node_service::internal::ResolvedNodeServices;

struct NodeServiceHolder {
  explicit NodeServiceHolder(const NodeServiceContext& node_service_context)
      : NodeServiceHolder{
            MakeDataServicesNodeServiceContext(node_service_context)} {}

  explicit NodeServiceHolder(
      const CoroutineNodeServiceContext& node_service_context)
      : session_service{node_service_context.session_service_},
        node_service{MakeNodeServiceImplContext(node_service_context)},
        node_service_notifier{node_service, session_service} {}

  explicit NodeServiceHolder(
      DataServicesNodeServiceContext&& node_service_context)
      : resolved_services{std::make_unique<ResolvedNodeServices>(
            node_service_context.executor_,
            std::move(node_service_context.data_services_))},
        session_service{*resolved_services->session_service},
        node_service{MakeNodeServiceImplContext(node_service_context)},
        node_service_notifier{node_service, session_service} {}

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
