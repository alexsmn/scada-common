#pragma once

#include "base/boost_log.h"
#include "base/observer_list.h"
#include "node_service/node_service.h"
#include "node_service/v1/address_space_fetcher_factory.h"
#include "node_service/v1/node_model_impl.h"

#include <map>

class AddressSpaceImpl;
class Executor;

namespace scada {
class Node;
struct ModelChangeEvent;
}  // namespace scada

namespace v1 {

class AddressSpaceFetcher;
class NodeModelImpl;
struct NodeFetchStatusChangedItem;

struct NodeServiceImplContext {
  const std::shared_ptr<Executor> executor_;
  const AddressSpaceFetcherFactory address_space_fetcher_factory_;
  scada::AttributeService& attribute_service_;
  AddressSpaceImpl& address_space_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::MethodService& method_service_;
};

class NodeServiceImpl final : private NodeServiceImplContext,
                              public NodeService,
                              private NodeModelDelegate,
                              private scada::NodeObserver {
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
  AddressSpaceFetcherFactoryContext MakeAddressSpaceFetcherFactoryContext();

  void OnNodeFetchStatusChanged(
      base::span<const NodeFetchStatusChangedItem> items);

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnSemanticChanged(const scada::SemanticChangeEvent& event);

  // NodeModelDelegate
  virtual NodeRef GetRemoteNode(const scada::Node* node) override;
  virtual void OnNodeModelDeleted(const scada::NodeId& node_id) override;
  virtual void OnNodeModelFetchRequested(
      const scada::NodeId& node_id,
      const NodeFetchStatus& requested_status) override;

  // scada::NodeObserver
  virtual void OnNodeCreated(const scada::Node& node) override;
  virtual void OnNodeDeleted(const scada::Node& node) override;
  virtual void OnNodeModified(const scada::Node& node,
                              const scada::PropertyIds& property_ids) override;
  virtual void OnReferenceAdded(const scada::ReferenceType& reference_type,
                                const scada::Node& source,
                                const scada::Node& target) override;
  virtual void OnReferenceDeleted(const scada::ReferenceType& reference_type,
                                  const scada::Node& source,
                                  const scada::Node& target) override;

  BoostLogger logger_{LOG_NAME("v1::NodeServiceImpl")};

  mutable base::ObserverList<NodeRefObserver> observers_;

  std::map<scada::NodeId, std::shared_ptr<NodeModelImpl>> nodes_;

  const std::shared_ptr<AddressSpaceFetcher> address_space_fetcher_;

  friend class NodeModelImpl;
};

}  // namespace v1
