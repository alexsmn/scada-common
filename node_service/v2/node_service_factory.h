#pragma once

#include <memory>

class NodeService;
struct CoroutineNodeServiceContext;
struct DataServicesNodeServiceContext;
struct NodeServiceContext;

namespace v2 {

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& context);

std::shared_ptr<NodeService> CreateNodeService(
    const CoroutineNodeServiceContext& context);

std::shared_ptr<NodeService> CreateNodeService(
    DataServicesNodeServiceContext&& context);

}  // namespace v2
