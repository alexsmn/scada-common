#pragma once

#include "base/boost_log.h"
#include "base/time/time.h"
#include "core/view_service.h"

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>

namespace boost::asio {
class io_context;
}

namespace scada {
class NodeId;
class Status;
class ViewService;
struct BrowseDescription;
struct BrowseResult;
}  // namespace scada

class Executor;

using ReferenceValidator = std::function<void(const scada::NodeId& node_id,
                                              scada::BrowseResult&& result)>;

struct NodeChildrenFetcherContext {
  boost::asio::io_context& io_context_;
  const std::shared_ptr<Executor> executor_;
  scada::ViewService& view_service_;
  const ReferenceValidator reference_validator_;
};

class NodeChildrenFetcher
    : private NodeChildrenFetcherContext,
      public std::enable_shared_from_this<NodeChildrenFetcher> {
 public:
  static std::shared_ptr<NodeChildrenFetcher> Create(
      NodeChildrenFetcherContext&& context);

  void Fetch(const scada::NodeId& node_id);
  void Cancel(const scada::NodeId& node_id);

 private:
  explicit NodeChildrenFetcher(NodeChildrenFetcherContext&& context);

  void FetchPendingNodes();

  void OnBrowseChildrenResult(
      base::TimeTicks start_ticks,
      scada::Status&& status,
      const std::vector<scada::BrowseDescription>& descriptions,
      std::vector<scada::BrowseResult>&& results);

  void FetchChildren(const std::vector<scada::NodeId>& node_ids);

  BoostLogger logger_{LOG_NAME("NodeChildrenFetcher")};

  size_t children_request_count_ = 0;
  std::deque<scada::NodeId> pending_children_;
  std::set<scada::NodeId> pending_children_set_;
};
