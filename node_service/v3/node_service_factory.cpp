#include "node_service/v3/node_service_factory.h"

#include "common/session_proxy_notifier.h"
#include "node_service/node_service_factory.h"
#include "node_service/node_service_factory_services.h"
#include "node_service/v3/node_service_impl.h"
#include "node_service/v3/service_node_fetcher.h"
#include "scada/service_context.h"

#include <memory>

namespace v3 {

namespace {
using node_service::internal::HasRequiredNodeServices;
using node_service::internal::MakeDataServicesNodeServiceContext;
using node_service::internal::ResolvedNodeServices;

// Builds the production coroutine fetcher v3::NodeServiceImpl injects. Like v2's
// internal NodeFetcherImpl, it runs with a default ServiceContext.
std::shared_ptr<NodeFetcher> MakeServiceNodeFetcher(
    scada::ViewService& view_service,
    scada::AttributeService& attribute_service) {
  return std::make_shared<ServiceNodeFetcher>(ServiceNodeFetcherContext{
      .view_service_ = view_service,
      .attribute_service_ = attribute_service,
      .service_context_ = scada::ServiceContext{},
  });
}

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
    return NodeServiceImplContext{
        .executor_ = node_service_context.executor_,
        .monitored_item_service_ = node_service_context.monitored_item_service_,
        .node_fetcher_ =
            MakeServiceNodeFetcher(node_service_context.view_service_,
                                   node_service_context.attribute_service_),
        .view_events_provider_ = MakeViewEventsProvider(
            node_service_context.executor_,
            node_service_context.monitored_item_service_),
    };
  }

  NodeServiceImplContext MakeNodeServiceImplContext(
      const DataServicesNodeServiceContext& node_service_context) {
    return NodeServiceImplContext{
        .executor_ = node_service_context.executor_,
        .monitored_item_service_ = *resolved_services->monitored_item_service,
        .node_fetcher_ =
            MakeServiceNodeFetcher(*resolved_services->view_service,
                                   *resolved_services->attribute_service),
        .view_events_provider_ = MakeViewEventsProvider(
            node_service_context.executor_,
            *resolved_services->monitored_item_service),
    };
  }

  std::unique_ptr<ResolvedNodeServices> resolved_services;
  scada::SessionService& session_service;
  NodeServiceImpl node_service;
  SessionProxyNotifier<NodeServiceImpl> node_service_notifier;
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
  if (!HasRequiredNodeServices(node_service_context.data_services_))
    return nullptr;

  auto context =
      std::make_shared<NodeServiceHolder>(std::move(node_service_context));
  return std::shared_ptr<NodeService>{context, &context->node_service};
}

}  // namespace v3
