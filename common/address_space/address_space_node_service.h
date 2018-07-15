#pragma once

#include <map>

#include "base/observer_list.h"
#include "common/address_space/address_space_fetcher.h"
#include "common/address_space/address_space_node_model.h"
#include "common/node_service.h"

namespace scada {
class AddressSpace;
class Node;
struct ModelChangeEvent;
}  // namespace scada

class AddressSpaceNodeModel;
class Logger;

struct AddressSpaceNodeServiceContext {
  const std::shared_ptr<Logger> logger_;
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
  virtual std::unique_ptr<scada::MonitoredItem> OnNodeModelCreateMonitoredItem(
      const scada::ReadValueId& read_value_id) override;
  virtual void OnNodeModelWrite(const scada::NodeId& node_id,
                                double value,
                                const scada::NodeId& user_id,
                                const scada::WriteFlags& flags,
                                const scada::StatusCallback& callback) override;
  virtual void OnNodeModelCall(const scada::NodeId& node_id,
                               const scada::NodeId& method_id,
                               const std::vector<scada::Variant>& arguments,
                               const scada::NodeId& user_id,
                               const scada::StatusCallback& callback) override;

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

  AddressSpaceFetcher fetcher_;

  friend class AddressSpaceNodeModel;
};
