#pragma once

#include "base/any_executor.h"
#include "events/view_events_subscription.h"
#include "node_service/node_children_fetcher.h"
#include "node_service/node_events.h"
#include "node_service/node_fetcher_impl.h"
#include "node_service/node_service.h"
#include "scada/view_service.h"

#include <list>
#include <unordered_map>

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

  // Bounds the service-owned MRU keep-alive pins that absorb refetch churn
  // from transient GetNode traversals. Residency beyond this window requires
  // callers to hold their NodeRefs.
  size_t keep_alive_capacity_ = 256;
};

// Node-model map shared between the service and its models. Models
// unregister themselves on destruction through this shared object, so a
// NodeRef released after the service has been torn down stays safe.
struct NodeModelRegistry {
  // Hashed: only ever point-accessed (find/insert/erase by NodeId), never
  // iterated in order.
  std::unordered_map<scada::NodeId, std::weak_ptr<NodeModelImpl>> nodes;
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

  virtual scada::Status GetStatus(const scada::NodeId& node_id) override;
  virtual NodeFetchStatus GetFetchStatus(const scada::NodeId& node_id) override;
  virtual Awaitable<void> Fetch(
      const scada::NodeId& node_id,
      const NodeFetchStatus& requested_status) override;
  virtual void StartFetch(const scada::NodeId& node_id,
                          const NodeFetchStatus& requested_status) override;
  virtual scada::Variant GetAttribute(const scada::NodeId& node_id,
                                      scada::AttributeId attribute_id) override;
  virtual NodeRef GetDataType(const scada::NodeId& node_id) override;
  virtual NodeRef::Reference GetReference(
      const scada::NodeId& node_id,
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& target_id) override;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& node_id,
      const scada::NodeId& reference_type_id,
      bool forward) override;
  virtual NodeRef GetTarget(const scada::NodeId& node_id,
                            const scada::NodeId& reference_type_id,
                            bool forward) override;
  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& node_id,
      const scada::NodeId& reference_type_id,
      bool forward) override;
  virtual NodeRef GetAggregate(
      const scada::NodeId& node_id,
      const scada::NodeId& aggregate_declaration_id) override;
  virtual NodeRef GetChild(const scada::NodeId& node_id,
                           const scada::QualifiedName& child_name) override;
  virtual scada::node GetScadaNode(const scada::NodeId& node_id) override;

  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeModelChanged(const scada::NodeId& node_id,
                        const ModelChangedCallback& callback) override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const scada::NodeId& node_id,
      const NodeSemanticChangedCallback& callback) override;
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const scada::NodeId& node_id,
      const NodeFetchedCallback& callback) override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(const scada::NodeId& node_id,
                            const NodeStateChangedCallback& callback) override;

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

  // Number of node models currently resident (pinned by NodeRefs, in-flight
  // fetches, parents, or the keep-alive window). Exposed for diagnostics and
  // residency tests; not part of the NodeService interface.
  size_t GetResidentNodeCount() const { return registry_->nodes.size(); }

 private:
  std::shared_ptr<NodeModelImpl> GetNodeModel(const scada::NodeId& node_id);

  // Like GetNodeModel but also kicks a NodeOnly fetch, so a data read on a
  // cursor whose entry was evicted transparently re-fetches (StartFetch is a
  // no-op once the node is already fetched).
  std::shared_ptr<NodeModelImpl> GetNodeModelForRead(
      const scada::NodeId& node_id);

  // Returns the model for |node_id| if one is already resident, without
  // creating it. Use for update/notification paths that must not grow the
  // node graph as a side effect.
  std::shared_ptr<NodeModelImpl> FindNodeModel(
      const scada::NodeId& node_id) const;

  // Moves |model| to the front of the bounded keep-alive window, possibly
  // releasing the least-recently-used model.
  void TouchKeepAlive(std::shared_ptr<NodeModelImpl> model);

  // Drops the keep-alive pin for |node_id| (e.g. when the node is deleted).
  void RemoveFromKeepAlive(const scada::NodeId& node_id);

  // Spawns the remote fetch coroutines for |node_id|. |model| is pinned by
  // the coroutines so the fetch target cannot be released mid-flight.
  void SpawnFetch(const scada::NodeId& node_id,
                  const NodeFetchStatus& requested_status,
                  std::shared_ptr<NodeModelImpl> model);

  void NotifyModelChanged(const scada::ModelChangeEvent& event);
  void NotifySemanticsChanged(const scada::NodeId& node_id);
  void NotifyNodeStateChanged(const NodeStateChangedEvent& event);

  void OnFetchNode(const scada::NodeId& node_id,
                   const NodeFetchStatus& requested_status);

  void ProcessFetchedNodes(std::vector<scada::NodeState>&& node_states);
  void ProcessFetchedChildren(const scada::NodeId& node_id,
                              scada::ReferenceDescriptions&& references);
  void ProcessFetchErrors(NodeFetchStatuses&& errors);

  // scada::ViewService
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(
      const scada::SemanticChangeEvent& event) override;

  NodeSignals signals_;

  // Models are owned by their holders (NodeRefs, parents, in-flight fetches,
  // the keep-alive window); the registry only tracks them weakly so unused
  // parts of the node graph are released as soon as the last pin drops.
  const std::shared_ptr<NodeModelRegistry> registry_ =
      std::make_shared<NodeModelRegistry>();

  // MRU keep-alive pins; see NodeServiceImplContext::keep_alive_capacity_.
  std::list<std::shared_ptr<NodeModelImpl>> keep_alive_list_;
  std::unordered_map<scada::NodeId,
                     std::list<std::shared_ptr<NodeModelImpl>>::iterator>
      keep_alive_index_;

  // Fetch requested before the channel opened. The model is pinned so the
  // requester's node survives until the fetch can actually start.
  struct PendingNodeFetch {
    NodeFetchStatus status;
    std::shared_ptr<NodeModelImpl> model;
  };

  bool channel_opened_ = false;
  std::map<scada::NodeId, PendingNodeFetch> pending_fetch_nodes_;

  std::unique_ptr<IViewEventsSubscription> view_events_subscription_;

  friend class NodeModelImpl;
};

}  // namespace v3
