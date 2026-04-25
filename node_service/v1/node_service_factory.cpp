#include "node_service/node_service_factory.h"

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "common/coroutine_session_proxy_notifier.h"
#include "node_service/node_service_factory.h"
#include "node_service/v1/address_space_fetcher_impl.h"
#include "node_service/v1/node_service_impl.h"
#include "scada/coroutine_services.h"

#include <memory>

namespace v1 {

namespace {

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

  NodeServiceImplContext MakeNodeServiceImplContext(
      const NodeServiceContext& node_service_context) {
    return NodeServiceImplContext{
        .address_space_fetcher_factory_ =
            MakeAddressSpaceFetcherFactory(node_service_context),
        .address_space_ = address_space,
        .scada_client_ = node_service_context.scada_client_};
  }

  NodeServiceImplContext MakeNodeServiceImplContext(
      const CoroutineNodeServiceContext& node_service_context) {
    return NodeServiceImplContext{
        .address_space_fetcher_factory_ =
            MakeAddressSpaceFetcherFactory(node_service_context),
        .address_space_ = address_space,
        .scada_client_ = node_service_context.scada_client_};
  }

  AddressSpaceFetcherFactory MakeAddressSpaceFetcherFactory(
      const NodeServiceContext& node_service_context) {
    return [this,
            node_service_context](AddressSpaceFetcherFactoryContext&& context) {
      auto view_events_provider =
          MakeViewEventsProvider(node_service_context.monitored_item_service_);

      return AddressSpaceFetcherImpl::Create(AddressSpaceFetcherImplContext{
          node_service_context.executor_,
          node_service_context.service_context_,
          *view_service_adapter,
          *attribute_service_adapter,
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

  AddressSpaceImpl address_space;

  // Don't create properties automatically, as server must provide valid
  // property IDs.
  GenericNodeFactory node_factory{address_space, false};

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

}  // namespace v1
