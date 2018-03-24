#pragma once

#include "base/observer_list.h"
#include "common/node_children_fetcher.h"
#include "common/node_fetcher.h"
#include "common/node_service.h"
#include "core/view_service.h"

#include <map>

namespace scada {
class AttributeService;
struct ModelChangeEvent;
}

class Logger;
class NodeFetcher;
class RemoteNodeModel;
struct NodeModelImplReference;

struct RemoteNodeServiceContext {
  const std::shared_ptr<Logger> logger_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
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

  NodeFetcherContext MakeNodeFetcherContext();
  NodeChildrenFetcherContext MakeNodeChildrenFetcherContext();

  // scada::ViewService
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(const scada::NodeId& node_id) override;

  mutable base::ObserverList<NodeRefObserver> observers_;

  std::map<scada::NodeId, std::shared_ptr<RemoteNodeModel>> nodes_;

  NodeFetcher node_fetcher_;
  NodeChildrenFetcher node_children_fetcher_;

  bool channel_opened_ = false;
  std::map<scada::NodeId, NodeFetchStatus> pending_fetch_nodes_;

  friend class RemoteNodeModel;
};
