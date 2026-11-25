#pragma once

#include "base/any_executor.h"
#include "common/aliases.h"
#include "scada/data_services.h"
#include "scada/node_id.h"

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

class NodeEventProvider;
class NodeService;

namespace scada {
class HistoryService;
}  // namespace scada

struct TimedDataContext {
  AnyExecutor executor_;
  const AliasResolver alias_resolver_;
  NodeService& node_service_;
  DataServices data_services_;
  std::shared_ptr<scada::HistoryService> history_service_;
  NodeEventProvider& node_event_provider_;
  // The connection's index for a namespace URI, for references stored in the
  // "nsu=" form (scada::NodeIdFromNodeIdString). Empty: canonical table only.
  std::function<std::optional<scada::NamespaceIndex>(std::string_view uri)>
      namespace_index_for_uri_ = {};
};

struct CoroutineTimedDataContext {
  AnyExecutor executor_;
  const AliasResolver alias_resolver_;
  NodeService& node_service_;
  std::shared_ptr<scada::HistoryService> history_service_;
  NodeEventProvider& node_event_provider_;
  // The connection's index for a namespace URI, for references stored in the
  // "nsu=" form (scada::NodeIdFromNodeIdString). Empty: canonical table only.
  std::function<std::optional<scada::NamespaceIndex>(std::string_view uri)>
      namespace_index_for_uri_ = {};
};
