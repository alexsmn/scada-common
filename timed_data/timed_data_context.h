#pragma once

#include "base/any_executor.h"
#include "common/aliases.h"
#include "scada/node_id.h"
#include "scada/services.h"

#include <memory>

class NodeEventProvider;
class NodeService;

struct TimedDataContext {
  AnyExecutor executor_;
  const AliasResolver alias_resolver_;
  NodeService& node_service_;
  const scada::services services_;
  NodeEventProvider& node_event_provider_;
};