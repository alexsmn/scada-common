#pragma once

#include "base/any_executor.h"
#include "base/observer_list.h"
#include "events/view_events_subscription.h"
#include "node_service/node_children_fetcher.h"
#include "node_service/node_fetcher_impl.h"
#include "node_service/node_service.h"
#include "scada/view_service.h"

#include <map>

namespace scada {
class MonitoredItemService;
struct ModelChangeEvent;
}  // namespace scada

namespace v3 {

class NodeFetcher;
class NodeModelImpl;
struct NodeModelImplReference;

struct NodeServiceImplContext {
  AnyExecutor executor_;
  scada::MonitoredItemService& monitored_item_service_;
  const std::shared_ptr<NodeFetcher> node_fetcher_;
  const ViewEventsProvider view_events_provider_;
};

// NodeService implementation using injected coroutine fetch logic.
//
// v3 keeps v2's direct per-node NodeState cache but moves fetch mechanics
// behind NodeFetcher. This makes the service smaller and easier to adapt to
// alternate data sources; unlike v2, it does not own NodeFetcherImpl or
// NodeChildrenFetcher directly.
class NodeServiceImpl : private NodeServiceImplContext,
                        private scada::ViewEvents,
                        public NodeService,
                        public std::enable_shared_from_this<NodeServiceImpl> {
 public:
  explicit NodeServiceImpl(NodeServiceImplContext&& context);
  ~NodeServiceImpl();

  void OnChannelOpened();
  void OnChannelClosed();

  // NodeService
  virtual NodeRef GetNode(const scada::NodeId& node_id) override;
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;
  virtual size_t GetPendingTaskCount() const override;

  // Number of node models currently held by the service. Exposed for
  // diagnostics and residency tests; not part of the NodeService interface.
  size_t GetResidentNodeCount() const { return nodes_.size(); }

 private:
  std::shared_ptr<NodeModelImpl> GetNodeModel(const scada::NodeId& node_id);

  // Returns the model for |node_id| if one is already resident, without
  // creating it. Use for update/notification paths that must not grow the
  // node graph as a side effect.
  std::shared_ptr<NodeModelImpl> FindNodeModel(
      const scada::NodeId& node_id) const;

  void NotifyModelChanged(const scada::ModelChangeEvent& event);
  void NotifySemanticsChanged(const scada::NodeId& node_id);

  void OnFetchNode(const scada::NodeId& node_id,
                   const NodeFetchStatus& requested_status);

  void ProcessFetchedNodes(std::vector<scada::NodeState>&& node_states);
  void ProcessFetchedChildren(
      const scada::NodeId& node_id,
      scada::ReferenceDescriptions&& references);
  void ProcessFetchErrors(NodeFetchStatuses&& errors);

  // scada::ViewService
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(
      const scada::SemanticChangeEvent& event) override;

  mutable base::ObserverList<NodeRefObserver> observers_;

  std::map<scada::NodeId, std::shared_ptr<NodeModelImpl>> nodes_;

  bool channel_opened_ = false;
  std::map<scada::NodeId, NodeFetchStatus> pending_fetch_nodes_;

  std::unique_ptr<IViewEventsSubscription> view_events_subscription_;

  friend class NodeModelImpl;
};

}  // namespace v3
