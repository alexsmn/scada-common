#include "node_service/node_service_factory.h"

#include "base/command_line.h"
#include "node_service/v1/node_service_factory.h"
#include "node_service/v2/node_service_factory.h"

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& context) {
  return base::CommandLine::ForCurrentProcess()->HasSwitch("node-service-v2")
             ? v1::CreateNodeService(context)
             : v2::CreateNodeService(context);
}
