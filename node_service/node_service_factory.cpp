#include "node_service/node_service_factory.h"

#include "node_service/v1/node_service_factory.h"
#include "node_service/v2/node_service_factory.h"

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& context,
    bool use_v2) {
  return use_v2 ? v2::CreateNodeService(context)
                : v1::CreateNodeService(context);
}
