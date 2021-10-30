#pragma once

#include "base/boost_log.h"
#include "node_service/fetch_queue.h"
#include "node_service/fetching_node_graph.h"
#include "node_service/node_fetcher.h"

#include <memory>
#include <vector>

namespace scada {
class AttributeService;
class DataValue;
class NodeId;
class Status;
class ViewService;
struct ReadValueId;
struct ServiceContext;
}  // namespace scada

class Executor;

struct NodeFetcherImplContext {
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

  // NodeFetcher
  virtual void Fetch(const scada::NodeId& node_id,
                     const NodeFetchStatus& status,
                     bool force = false) override;
  virtual void Cancel(const scada::NodeId& node_id) override;
  virtual size_t GetPendingNodeCount() const override;

 private:
  explicit NodeFetcherImpl(NodeFetcherImplContext&& context);

  unsigned MakeRequestId();

  void FetchNode(FetchingNode& node,
                 unsigned pending_sequence,
                 const NodeFetchStatus& status,
                 bool force);

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

  void ApplyReadResult(unsigned request_id,
                       const scada::ReadValueId& read_id,
                       scada::DataValue&& result);

  void ApplyBrowseResult(unsigned request_id,
                         const scada::BrowseDescription& description,
                         scada::BrowseResult&& result);

  void ValidateDependency(FetchingNode& node, const scada::NodeId& from_id);

  bool AssertValid() const;

  BoostLogger logger_{LOG_NAME("NodeFetcher")};

  FetchingNodeGraph fetching_nodes_;

  size_t running_request_count_ = 0;

  // Can't be zero.
  unsigned next_request_id_ = 1;

  FetchQueue pending_queue_;

  unsigned next_pending_sequence_ = 0;

  // Blocks |FetchPendingNodes()| so |OnReadResult()| and |OnBrowseResult()| may
  // not be called recursively.
  bool processing_response_ = false;
};
