#include "node_service/v2/node_service_factory.h"

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "node_service/node_service_factory.h"
#include "node_service/v2/node_service_impl.h"
#include "remote/session_proxy_notifier.h"

namespace v2 {

namespace {

struct NodeServiceHolder {
  explicit NodeServiceHolder(const NodeServiceContext& node_service_context)
      : node_service{MakeNodeServiceImplContext(node_service_context)},
        node_service_notifier{node_service,
                              node_service_context.session_service_} {}

  NodeServiceImplContext MakeNodeServiceImplContext(
      const NodeServiceContext& node_service_context) {
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

  NodeServiceImpl node_service;
  SessionProxyNotifier<NodeServiceImpl> node_service_notifier;
};

}  // namespace

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& node_service_context) {
  auto context = std::make_shared<NodeServiceHolder>(node_service_context);
  return std::shared_ptr<NodeService>{context, &context->node_service};
}

}  // namespace v2
