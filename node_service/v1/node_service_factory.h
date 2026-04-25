#pragma once

#include <memory>

class NodeService;
struct CoroutineNodeServiceContext;
struct NodeServiceContext;

namespace v1 {

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& context);

std::shared_ptr<NodeService> CreateNodeService(
    const CoroutineNodeServiceContext& context);

}  // namespace v1
