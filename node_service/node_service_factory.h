#pragma once

#include "base/any_executor.h"
#include "scada/client.h"

#include <memory>

namespace scada {
class AttributeService;
class CoroutineAttributeService;
class CoroutineSessionService;
class CoroutineViewService;
class MethodService;
class MonitoredItemService;
class ServiceContext;
class SessionService;
class ViewService;
}  // namespace scada

class NodeService;

struct NodeServiceContext {
  AnyExecutor executor_;
  const scada::ServiceContext service_context_;
  // TODO: Remove services and keep `scada::client` only.
  scada::SessionService& session_service_;
  scada::AttributeService& attribute_service_;
  scada::ViewService& view_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::MethodService& method_service_;
  scada::client scada_client_;
};

struct CoroutineNodeServiceContext {
  AnyExecutor executor_;
  const scada::ServiceContext service_context_;
  // TODO: Remove services and keep `scada::client` only.
  scada::CoroutineSessionService& session_service_;
  scada::CoroutineAttributeService& attribute_service_;
  scada::CoroutineViewService& view_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::client scada_client_;
};

std::shared_ptr<NodeService> CreateNodeService(
    const NodeServiceContext& context,
    bool use_v2 = false);

std::shared_ptr<NodeService> CreateNodeService(
    const CoroutineNodeServiceContext& context,
    bool use_v2 = false);
