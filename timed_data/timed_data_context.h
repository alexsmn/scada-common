#pragma once

#include "common/aliases.h"
#include "core/node_id.h"
#include "core/service.h"

#include <memory>

class Executor;
class NodeEventProvider;
class NodeService;

struct TimedDataContext {
  const std::shared_ptr<Executor> executor_;
  const AliasResolver alias_resolver_;
  NodeService& node_service_;
  const scada::services services_;
  NodeEventProvider& node_event_provider_;
};