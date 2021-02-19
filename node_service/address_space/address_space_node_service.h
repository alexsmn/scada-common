#pragma once

#include "base/observer_list.h"
#include "common/view_events_subscription.h"
#include "node_service/address_space/address_space_fetcher.h"
#include "node_service/address_space/address_space_node_model.h"
#include "node_service/node_service.h"

#include <map>

class Executor;

namespace boost::asio {
class io_context;
}

namespace scada {
class AddressSpace;
class Node;
struct ModelChangeEvent;
}  // namespace scada

class AddressSpaceNodeModel;

struct AddressSpaceNodeServiceContext {
  boost::asio::io_context& io_context_;
  const std::shared_ptr<Executor> executor_;
  const ViewEventsProvider view_events_provider_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  AddressSpaceImpl& address_space_;
  NodeFactory& node_factory_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::MethodService& method_service_;
};

class AddressSpaceNodeService final : private AddressSpaceNodeServiceContext,
                                      public NodeService,
                                      private AddressSpaceNodeModelDelegate,
                                      private scada::NodeObserver {
 public:
  explicit AddressSpaceNodeService(AddressSpaceNodeServiceContext&& context);
  ~AddressSpaceNodeService();

  void OnChannelOpened();
  void OnChannelClosed();

  // NodeService
  virtual NodeRef GetNode(const scada::NodeId& node_id) override;
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;
  virtual size_t GetPendingTaskCount() const override;

 private:
  AddressSpaceFetcherContext MakeAddressSpaceFetcherContext();

  void OnNodeFetchStatusChanged(const scada::NodeId& node_id,
                                const scada::Status& status,
                                const NodeFetchStatus& fetch_status);

  void OnModelChanged(const scada::ModelChangeEvent& event);

  // AddressSpaceNodeModelDelegate
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

  mutable base::ObserverList<NodeRefObserver> observers_;

  std::map<scada::NodeId, std::shared_ptr<AddressSpaceNodeModel>> nodes_;

  const std::shared_ptr<AddressSpaceFetcher> fetcher_;

  friend class AddressSpaceNodeModel;
};
