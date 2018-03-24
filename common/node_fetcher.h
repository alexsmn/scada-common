#pragma once

#include "common/node_state.h"
#include "core/configuration_types.h"
#include "core/data_value.h"

#include <functional>
#include <map>
#include <memory>
#include <queue>

namespace scada {
class AttributeService;
class NodeId;
class Status;
class ViewService;
}  // namespace scada

class Logger;

using NodeValidator = std::function<bool(const scada::NodeId& node_id)>;

struct NodeFetcherContext {
  Logger& logger_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  const std::function<void(std::vector<scada::NodeState>&& nodes)>
      fetch_completed_handler_;
  const NodeValidator node_validator_;
};

class NodeFetcher : private NodeFetcherContext {
 public:
  explicit NodeFetcher(NodeFetcherContext&& context);
  ~NodeFetcher();

  void Fetch(const scada::NodeId& node_id,
             bool fetch_parent = false,
             const scada::NodeId& parent_id = {},
             const scada::NodeId& reference_type_id = {});
  void Cancel(const scada::NodeId& node_id);

 private:
  struct FetchingNode : scada::NodeState {
    bool fetched() const { return attributes_fetched && references_fetched; }

    void ClearDependsOf();
    void ClearDependentNodes();

    bool pending = false;           // the node is in |pending_queue_|.
    bool fetch_started = false;     // requests were sent (can be completed).
    unsigned fetch_request_id = 0;  // for request cancelation
    bool fetch_parent = false;
    std::vector<FetchingNode*> depends_of;
    std::vector<FetchingNode*> dependent_nodes;
    bool attributes_fetched = false;
    bool references_fetched = false;
    scada::Status status{scada::StatusCode::Good};
  };

  class FetchingNodeGraph {
   public:
    std::size_t size() const { return fetching_nodes_.size(); }

    FetchingNode* FindNode(const scada::NodeId& node_id);
    FetchingNode& AddNode(const scada::NodeId& node_id);

    void RemoveNode(const scada::NodeId& node_id);

    void AddDependency(FetchingNode& node, FetchingNode& from);

    std::vector<scada::NodeState> GetFetchedNodes();

    bool AssertValid() const;

   private:
    std::map<scada::NodeId, FetchingNode> fetching_nodes_;

    friend class NodeFetcher;
  };

  void FetchNode(FetchingNode& node);

  void FetchPendingNodes();
  void FetchPendingNodes(std::vector<FetchingNode*>&& nodes);

  void NotifyFetchedNodes();

  void OnFetchError(scada::Status&& status);

  void AddFetchedReference(FetchingNode& node,
                           const scada::BrowseDescription& description,
                           scada::ReferenceDescription&& reference);

  void OnReadResult(unsigned request_id, base::TimeTicks start_ticks,
                    scada::Status&& status,
                    const std::vector<scada::ReadValueId>& read_ids,
                    std::vector<scada::DataValue>&& results);
  void OnBrowseResult(unsigned request_id, base::TimeTicks start_ticks,
                      scada::Status&& status,
                      const std::vector<scada::BrowseDescription>& descriptions,
                      std::vector<scada::BrowseResult>&& results);

  void ValidateDependency(FetchingNode& node, const scada::NodeId& from_id);

  bool AssertValid() const;

  FetchingNodeGraph fetching_nodes_;

  size_t running_request_count_ = 0;

  // Can't be zero.
  unsigned next_request_id_ = 1;

  std::deque<FetchingNode*> pending_queue_;
};
