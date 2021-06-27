#pragma once

#include "base/boost_log.h"
#include "base/observer_list.h"
#include "common/view_events_subscription.h"
#include "core/view_service.h"
#include "node_service/node_children_fetcher.h"
#include "node_service/node_fetcher_impl.h"
#include "node_service/node_service.h"

#include <map>

class NodeFetcher;

namespace scada {
class AttributeService;
class MonitoredItemService;
struct ModelChangeEvent;
}  // namespace scada

namespace v2 {

class NodeModelImpl;
struct NodeModelImplReference;

struct NodeServiceImplContext {
  const std::shared_ptr<Executor> executor_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  scada::MonitoredItemService& monitored_item_service_;
};

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

 private:
  std::shared_ptr<NodeModelImpl> GetNodeModel(const scada::NodeId& node_id);

  void NotifyModelChanged(const scada::ModelChangeEvent& event);
  void NotifySemanticsChanged(const scada::NodeId& node_id);

  void OnFetchNode(const scada::NodeId& node_id,
                   const NodeFetchStatus& requested_status);

  void ProcessFetchedNodes(std::vector<scada::NodeState>&& node_states);
  void ProcessFetchErrors(NodeFetchStatuses&& errors);

  NodeFetcherImplContext MakeNodeFetcherImplContext();
  NodeChildrenFetcherContext MakeNodeChildrenFetcherContext();

  // scada::ViewService
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(
      const scada::SemanticChangeEvent& event) override;

  BoostLogger logger_{LOG_NAME("v2::NodeService")};

  mutable base::ObserverList<NodeRefObserver> observers_;

  std::map<scada::NodeId, std::shared_ptr<NodeModelImpl>> nodes_;

  const std::shared_ptr<NodeFetcherImpl> node_fetcher_;
  const std::shared_ptr<NodeChildrenFetcher> node_children_fetcher_;

  bool channel_opened_ = false;
  std::map<scada::NodeId, NodeFetchStatus> pending_fetch_nodes_;

  ViewEventsSubscription view_events_subscription_{monitored_item_service_,
                                                   *this};

  friend class NodeModelImpl;
};

}  // namespace v2
