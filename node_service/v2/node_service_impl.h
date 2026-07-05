#pragma once

#include "base/any_executor.h"
#include "base/boost_log.h"
#include "events/view_events_subscription.h"
#include "node_service/node_children_fetcher.h"
#include "node_service/node_events.h"
#include "node_service/node_fetcher_impl.h"
#include "node_service/node_service.h"
#include "remote/view_event_queue.h"
#include "scada/view_service.h"

#include <map>
#include <memory>

class NodeFetcher;

namespace scada {
class AttributeService;
class ViewService;
class MonitoredItemService;
struct ModelChangeEvent;
}  // namespace scada

namespace v2 {

class NodeServiceImpl;
class NodeModelImpl;
struct NodeModelImplReference;

struct NodeServiceImplContext {
  AnyExecutor executor_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  scada::MonitoredItemService& monitored_item_service_;
  const ViewEventsProvider view_events_provider_;
};

// Small RAII helper used by v2 to defer event delivery while model state is
// updated in a batch.
template <class Object>
class ScopedLock {
 public:
  explicit ScopedLock(Object& object) : object_{object} { object_.Lock(); }
  ~ScopedLock() { object_.Unlock(); }

 private:
  Object& object_;
};

// Queues v2 model and semantic events while nodes are being fetched or updated,
// then replays them after the service leaves the locked section.
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

// NodeService implementation backed by per-node NodeState caches.
//
// v2 removes v1's mutable AddressSpace mirror. NodeModelImpl stores fetched
// attributes and references directly, while the service owns the standard
// NodeFetcherImpl and NodeChildrenFetcher. v3 keeps the same direct-cache idea
// but injects its fetcher and simplifies event/pending state handling.
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
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeModelChanged(const ModelChangedCallback& callback) const override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const NodeSemanticChangedCallback& callback) const override;
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const NodeFetchedCallback& callback) const override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(
      const NodeStateChangedCallback& callback) const override;
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

  NodeSignals signals_;

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
