#pragma once

#include <memory>

class NodeService;
struct NodeServiceContext;

namespace v2 {

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& context);

}  // namespace v2
