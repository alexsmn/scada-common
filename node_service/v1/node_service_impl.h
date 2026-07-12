#pragma once

#include "base/boost_log.h"
#include "node_service/node_events.h"
#include "node_service/node_service.h"
#include "node_service/node_subscription_table.h"
#include "node_service/v1/address_space_fetcher_factory.h"
#include "node_service/v1/node_model_impl.h"

#include <boost/signals2/connection.hpp>
#include <map>

namespace scada {
class AddressSpace;
class Node;
struct ModelChangeEvent;
}  // namespace scada

namespace v1 {

class AddressSpaceFetcher;
class NodeModelImpl;
struct NodeFetchStatusChangedItem;

struct NodeServiceImplContext {
  const AddressSpaceFetcherFactory address_space_fetcher_factory_;
  scada::AddressSpace& address_space_;
  scada::client scada_client_;
};

// NodeService implementation backed by a mutable AddressSpace mirror.
//
// v1 keeps a local scada::AddressSpace up to date through AddressSpaceFetcher
// and exposes NodeRef objects over that mirrored graph. Compared with v2/v3,
// this version has the heaviest local model: fetched remote NodeState data is
// materialized as address-space nodes before NodeModelImpl serves attributes
// and references.
class NodeServiceImpl final : private NodeServiceImplContext,
                              public NodeService,
                              private NodeModelDelegate {
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

 private:
  std::shared_ptr<NodeModelImpl> GetNodeModel(const scada::NodeId& node_id);

  AddressSpaceFetcherFactoryContext MakeAddressSpaceFetcherFactoryContext();

  void OnNodeFetchStatusChanged(
      std::span<const NodeFetchStatusChangedItem> items);

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnSemanticChanged(const scada::SemanticChangeEvent& event);

  // NodeModelDelegate
  virtual NodeRef GetRemoteNode(const scada::Node* node) override;
  virtual void OnNodeModelDeleted(const scada::NodeId& node_id) override;
  virtual void OnNodeModelFetchRequested(
      const scada::NodeId& node_id,
      const NodeFetchStatus& requested_status) override;

  void OnNodeDeleted(const scada::Node& node);

  BoostLogger logger_{LOG_NAME("v1::NodeServiceImpl")};

  // Service-wide observers (all nodes).
  NodeSignals signals_;

  // Per-node change subscriptions, materialized only for subscribed nodes.
  NodeSubscriptionTable subscription_table_;

  std::map<scada::NodeId, std::shared_ptr<NodeModelImpl>> nodes_;

  const std::shared_ptr<AddressSpaceFetcher> address_space_fetcher_;

  boost::signals2::scoped_connection node_deleted_connection_;

  friend class NodeModelImpl;
};

}  // namespace v1
