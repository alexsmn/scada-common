#pragma once

#include "base/boost_log.h"
#include "base/observer_list.h"
#include "common/view_events_subscription.h"
#include "scada/view_service.h"
#include "node_service/node_children_fetcher.h"
#include "node_service/node_fetcher_impl.h"
#include "node_service/node_service.h"
#include "remote/view_event_queue.h"

#include <map>

class NodeFetcher;

namespace scada {
class AttributeService;
class MonitoredItemService;
struct ModelChangeEvent;
}  // namespace scada

namespace v2 {

class NodeServiceImpl;
class NodeModelImpl;
struct NodeModelImplReference;

struct NodeServiceImplContext {
  const std::shared_ptr<Executor> executor_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  scada::MonitoredItemService& monitored_item_service_;
  const ViewEventsProvider view_events_provider_;
};

template <class Object>
class ScopedLock {
 public:
  explicit ScopedLock(Object& object) : object_{object} { object_.Lock(); }
  ~ScopedLock() { object_.Unlock(); }

 private:
  Object& object_;
};

class PendingEvents {
 public:
  explicit PendingEvents(NodeServiceImpl& service);

  void Lock();
  void Unlock();

  void PostEvent(const scada::ModelChangeEvent& event);
  void PostEvent(const scada::SemanticChangeEvent& event);

 private:
  void FireEvent(const scada::ModelChangeEvent& event);
  void FireEvent(const scada::SemanticChangeEvent& event);

  NodeServiceImpl& service_;

  ViewEventQueue event_queue_;

  int lock_count_ = 0;
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
  void OnNodeFetchStatusChanged(const scada::NodeId& node_id,
                                const scada::Status& status,
                                const NodeFetchStatus& fetch_status);

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
  const std::unique_ptr<IViewEventsSubscription> view_events_subscription_;

  bool channel_opened_ = false;
  std::map<scada::NodeId, NodeFetchStatus> pending_fetch_nodes_;

  PendingEvents pending_events_{*this};

  friend class NodeModelImpl;
  friend class PendingEvents;
};

}  // namespace v2
