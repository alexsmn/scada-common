#pragma once

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "core/node_id.h"

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <set>

namespace scada {
class Status;
class ViewService;
struct BrowseDescription;
struct BrowseResult;
}  // namespace scada

class Logger;

using ReferenceMap =
    std::map<scada::NodeId /*reference_type_id*/,
             std::map<scada::NodeId /*target_id*/,
                      scada::NodeId /*child_reference_type_id*/>>;
using ReferenceValidator =
    std::function<void(const scada::NodeId& node_id, ReferenceMap references)>;

struct NodeChildrenFetcherContext {
  const std::shared_ptr<Logger> logger_;
  scada::ViewService& view_service_;
  const ReferenceValidator reference_validator_;
};

class NodeChildrenFetcher : private NodeChildrenFetcherContext {
 public:
  explicit NodeChildrenFetcher(NodeChildrenFetcherContext&& context);

  void Fetch(const scada::NodeId& node_id);
  void Cancel(const scada::NodeId& node_id);

 private:
  void FetchPendingNodes();

  void OnBrowseChildrenResult(
      base::TimeTicks start_ticks,
      scada::Status&& status,
      const std::vector<scada::BrowseDescription>& descriptions,
      std::vector<scada::BrowseResult>&& results);

  void FetchChildren(const std::vector<scada::NodeId>& node_ids);

  size_t children_request_count_ = 0;
  std::deque<scada::NodeId> pending_children_;
  std::set<scada::NodeId> pending_children_set_;

  base::WeakPtrFactory<NodeChildrenFetcher> weak_factory_{this};
};
