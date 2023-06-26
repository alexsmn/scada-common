#pragma once

#include "core/client.h"

#include <memory>

namespace scada {
class AttributeService;
class MethodService;
class MonitoredItemService;
class SessionService;
class ViewService;
}  // namespace scada

class Executor;
class NodeService;

struct NodeServiceContext {
  const std::shared_ptr<Executor> executor;
  // TODO: Remove services and keep `scada::client` only.
  scada::SessionService& session_service_;
  scada::AttributeService& attribute_service_;
  scada::ViewService& view_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::MethodService& method_service_;
  scada::client scada_client_;
};

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& context);
