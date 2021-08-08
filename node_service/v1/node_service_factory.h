#pragma once

#include <memory>

class NodeService;
struct NodeServiceContext;

namespace v1 {

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& context);

}  // namespace v1
