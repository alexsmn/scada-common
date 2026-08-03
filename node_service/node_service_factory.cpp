#include "node_service/node_service_factory.h"

#include "node_service/v3/node_service_factory.h"

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& context) {
  return v3::CreateNodeService(context);
}

std::shared_ptr<NodeService> CreateNodeService(
    const CoroutineNodeServiceContext& context) {
  return v3::CreateNodeService(context);
}

std::shared_ptr<NodeService> CreateNodeService(
    DataServicesNodeServiceContext&& context) {
  return v3::CreateNodeService(std::move(context));
}
