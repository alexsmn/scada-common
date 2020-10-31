#pragma once

#include "base/observer_list.h"
#include "common/view_events_subscription.h"
#include "core/view_service.h"
#include "node_service/node_children_fetcher.h"
#include "node_service/node_fetcher_impl.h"
#include "node_service/node_service.h"

#include <map>

namespace scada {
class AttributeService;
struct ModelChangeEvent;
}  // namespace scada

class Logger;
class NodeFetcher;
class RemoteNodeModel;
struct NodeModelImplReference;

struct RemoteNodeServiceContext {
  const std::shared_ptr<Logger> logger_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  scada::MonitoredItemService& monitored_item_service_;
};

class RemoteNodeService : private RemoteNodeServiceContext,
                          private scada::ViewEvents,
                          public NodeService {
 public:
  explicit RemoteNodeService(RemoteNodeServiceContext&& context);
  ~RemoteNodeService();

  void OnChannelOpened();
  void OnChannelClosed();

  // NodeService
  virtual NodeRef GetNode(const scada::NodeId& node_id) override;
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;

 private:
  // Returns fetched or unfetched node impl.
  std::shared_ptr<RemoteNodeModel> GetNodeImpl(const scada::NodeId& node_id);

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

  mutable base::ObserverList<NodeRefObserver> observers_;

  std::map<scada::NodeId, std::shared_ptr<RemoteNodeModel>> nodes_;

  NodeFetcherImpl node_fetcher_;
  NodeChildrenFetcher node_children_fetcher_;

  bool channel_opened_ = false;
  std::map<scada::NodeId, NodeFetchStatus> pending_fetch_nodes_;

  ViewEventsSubscription view_events_subscription_{monitored_item_service_,
                                                   *this};

  friend class RemoteNodeModel;
};
