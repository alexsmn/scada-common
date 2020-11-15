#pragma once

#include "base/boost_log.h"
#include "core/attribute_service.h"
#include "core/data_value.h"
#include "node_service/node_fetcher.h"

#include <map>
#include <memory>
#include <queue>

namespace boost::asio {
class io_context;
}

namespace scada {
class AttributeService;
class NodeId;
class Status;
class ViewService;
struct ServiceContext;
}  // namespace scada

class Executor;

struct NodeFetcherImplContext {
  boost::asio::io_context& io_context_;
  const std::shared_ptr<Executor> executor_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  const FetchCompletedHandler fetch_completed_handler_;
  const NodeValidator node_validator_;
  const std::shared_ptr<const scada::ServiceContext> service_context_;
};

class NodeFetcherImpl : private NodeFetcherImplContext,
                        public NodeFetcher,
                        public std::enable_shared_from_this<NodeFetcherImpl> {
 public:
  ~NodeFetcherImpl();

  static std::shared_ptr<NodeFetcherImpl> Create(
      NodeFetcherImplContext&& context);

  virtual void Fetch(const scada::NodeId& node_id,
                     bool fetch_parent = false,
                     const std::optional<ParentInfo> parent_info = {},
                     bool force = false) override;
  virtual void Cancel(const scada::NodeId& node_id) override;

 private:
  explicit NodeFetcherImpl(NodeFetcherImplContext&& context);

  struct FetchingNode : scada::NodeState {
    bool fetched() const { return attributes_fetched && references_fetched; }

    void ClearDependsOf();
    void ClearDependentNodes();

    bool pending = false;  // the node is in |pending_queue_|.
    unsigned pending_sequence = 0;

    bool fetch_started = false;     // requests were sent (can be completed).
    unsigned fetch_request_id = 0;  // for request cancelation
    bool fetch_parent = false;
    std::vector<FetchingNode*> depends_of;
    std::vector<FetchingNode*> dependent_nodes;
    bool attributes_fetched = false;
    bool references_fetched = false;
    scada::Status status{scada::StatusCode::Good};
    bool force = false;  // refetch even if fetching has been already started
    bool is_property = false;  // instance or type declaration property
    bool is_declaration = false;

    int fetch_cache_iteration = 0;
    int fetch_cache_state = false;
  };

  class FetchingNodeGraph {
   public:
    std::size_t size() const { return fetching_nodes_.size(); }

    FetchingNode* FindNode(const scada::NodeId& node_id);
    FetchingNode& AddNode(const scada::NodeId& node_id);

    void RemoveNode(const scada::NodeId& node_id);

    void AddDependency(FetchingNode& node, FetchingNode& from);

    std::pair<std::vector<scada::NodeState> /*fetched_nodes*/,
              NodeFetchStatuses /*errors*/>
    GetFetchedNodes();

    bool AssertValid() const;

    std::string GetDebugString() const;

   private:
    std::map<scada::NodeId, FetchingNode> fetching_nodes_;

    int fetch_cache_iteration_ = 1;

    friend class NodeFetcherImpl;
  };

  class PendingQueue {
   public:
    bool empty() const { return queue_.empty(); };
    std::size_t size() const { return queue_.size(); }

    FetchingNode& top() { return *queue_.front().node; }

    std::size_t count(const FetchingNode& node) const;

    void push(FetchingNode& node);
    void pop();

    void erase(FetchingNode& node);

   private:
    struct Node {
      auto key() const { return std::tie(node->pending_sequence, sequence); }

      bool operator<(const Node& other) const { return key() > other.key(); }

      unsigned sequence = 0;
      FetchingNode* node = nullptr;
    };

    std::vector<Node>::iterator find(const FetchingNode& node);
    std::vector<Node>::const_iterator find(const FetchingNode& node) const;

    std::vector<Node> queue_;
    unsigned next_sequence_ = 0;
  };

  void FetchNode(FetchingNode& node, unsigned priority);

  void FetchPendingNodes();
  void FetchPendingNodes(std::vector<FetchingNode*>&& nodes);

  void NotifyFetchedNodes();

  void SetFetchedAttribute(FetchingNode& node,
                           scada::AttributeId attribute_id,
                           scada::Variant&& value);

  void AddFetchedReference(FetchingNode& node,
                           const scada::BrowseDescription& description,
                           scada::ReferenceDescription&& reference);

  void OnReadResult(unsigned request_id,
                    base::TimeTicks start_ticks,
                    scada::Status&& status,
                    const std::vector<scada::ReadValueId>& read_ids,
                    std::vector<scada::DataValue>&& results);
  void OnBrowseResult(unsigned request_id,
                      base::TimeTicks start_ticks,
                      scada::Status&& status,
                      const std::vector<scada::BrowseDescription>& descriptions,
                      std::vector<scada::BrowseResult>&& results);

  void ValidateDependency(FetchingNode& node, const scada::NodeId& from_id);

  bool AssertValid() const;

  static std::vector<scada::NodeId> CollectNodeIds(
      const std::vector<FetchingNode*> nodes);

  BoostLogger logger_{LOG_NAME("NodeFetcher")};

  FetchingNodeGraph fetching_nodes_;

  size_t running_request_count_ = 0;

  // Can't be zero.
  unsigned next_request_id_ = 1;

  PendingQueue pending_queue_;

  unsigned next_pending_sequence_ = 0;
};
