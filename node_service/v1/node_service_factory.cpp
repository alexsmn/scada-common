#include "node_service/node_service_factory.h"

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "common/coroutine_session_proxy_notifier.h"
#include "node_service/node_service_factory.h"
#include "node_service/node_service_factory_services.h"
#include "node_service/v1/address_space_fetcher_impl.h"
#include "node_service/v1/node_service_impl.h"

#include <memory>

namespace v1 {

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
    return NodeServiceImplContext{
        .address_space_fetcher_factory_ =
            MakeAddressSpaceFetcherFactory(node_service_context),
        .address_space_ = address_space,
        .scada_client_ = node_service_context.scada_client_};
  }

  NodeServiceImplContext MakeNodeServiceImplContext(
      const DataServicesNodeServiceContext& node_service_context) {
    return NodeServiceImplContext{
        .address_space_fetcher_factory_ =
            MakeAddressSpaceFetcherFactory(node_service_context),
        .address_space_ = address_space,
        .scada_client_ = node_service_context.scada_client_};
  }

  AddressSpaceFetcherFactory MakeAddressSpaceFetcherFactory(
      const CoroutineNodeServiceContext& node_service_context) {
    return [this,
            node_service_context](AddressSpaceFetcherFactoryContext&& context) {
      auto view_events_provider =
          MakeViewEventsProvider(node_service_context.monitored_item_service_);

      return AddressSpaceFetcherImpl::Create(AddressSpaceFetcherImplContext{
          node_service_context.executor_,
          node_service_context.service_context_,
          node_service_context.view_service_,
          node_service_context.attribute_service_,
          address_space,
          node_factory,
          std::move(view_events_provider),
          std::move(context.node_fetch_status_changed_handler_),
          std::move(context.model_changed_handler_),
          std::move(context.semantic_changed_handler_),
      });
    };
  }

  AddressSpaceFetcherFactory MakeAddressSpaceFetcherFactory(
      const DataServicesNodeServiceContext& node_service_context) {
    return [this,
            node_service_context](AddressSpaceFetcherFactoryContext&& context) {
      auto view_events_provider =
          MakeViewEventsProvider(*resolved_services->monitored_item_service);

      return AddressSpaceFetcherImpl::Create(AddressSpaceFetcherImplContext{
          node_service_context.executor_,
          node_service_context.service_context_,
          *resolved_services->view_service,
          *resolved_services->attribute_service,
          address_space,
          node_factory,
          std::move(view_events_provider),
          std::move(context.node_fetch_status_changed_handler_),
          std::move(context.model_changed_handler_),
          std::move(context.semantic_changed_handler_),
      });
    };
  }

  AddressSpaceImpl address_space;

  // Don't create properties automatically, as server must provide valid
  // property IDs.
  GenericNodeFactory node_factory{address_space, false};

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

}  // namespace v1
