#pragma once

#include "base/any_executor.h"
#include "common/aliases.h"
#include "scada/node_id.h"
#include "scada/services.h"

#include <memory>

class NodeEventProvider;
class NodeService;

namespace scada {
class CoroutineHistoryService;
}  // namespace scada

struct TimedDataContext {
  AnyExecutor executor_;
  const AliasResolver alias_resolver_;
  NodeService& node_service_;
  const scada::services services_;
  std::shared_ptr<scada::CoroutineHistoryService> history_service_;
  NodeEventProvider& node_event_provider_;
};

struct CoroutineTimedDataContext {
  AnyExecutor executor_;
  const AliasResolver alias_resolver_;
  NodeService& node_service_;
  std::shared_ptr<scada::CoroutineHistoryService> history_service_;
  NodeEventProvider& node_event_provider_;
};
