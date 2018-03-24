#include "common/address_space_node_service.h"

#include "common/address_space_node_model.h"
#include "common/node_observer.h"
#include "core/configuration.h"
#include "core/configuration_utils.h"
#include "core/node_utils.h"

// AddressSpaceNodeService

AddressSpaceNodeService::AddressSpaceNodeService(
    AddressSpaceNodeServiceContext&& context)
    : AddressSpaceNodeServiceContext{std::move(context)} {
  address_space_.Subscribe(*this);
}

AddressSpaceNodeService::~AddressSpaceNodeService() {
  address_space_.Unsubscribe(*this);
}

NodeRef AddressSpaceNodeService::GetNode(const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  auto& weak_model = nodes_[node_id];
  auto model = weak_model.lock();
  if (model)
    return model;

  auto& delegate = *static_cast<AddressSpaceNodeModelDelegate*>(this);
  model = std::make_shared<AddressSpaceNodeModel>(delegate, node_id);

  auto* node = address_space_.GetNode(node_id);
  const auto fetch_status = node_fetch_status_checker_(node_id);
  model->OnNodeFetchStatusChanged(node, fetch_status);

  if (fetch_status != NodeFetchStatus::Max())
    node_fetch_handler_(node_id, NodeFetchStatus::Max());

  weak_model = model;
  return model;
}

void AddressSpaceNodeService::Subscribe(NodeRefObserver& observer) const {
  observers_.AddObserver(&observer);
}

void AddressSpaceNodeService::Unsubscribe(NodeRefObserver& observer) const {
  observers_.RemoveObserver(&observer);
}

void AddressSpaceNodeService::OnModelChanged(
    const scada::ModelChangeEvent& event) {
  for (auto& o : observers_)
    o.OnModelChanged(event);

  if (auto i = nodes_.find(event.node_id); i != nodes_.end()) {
    if (auto ptr = i->second.lock())
      ptr->OnModelChanged(event);
  }
}

void AddressSpaceNodeService::OnNodeCreated(const scada::Node& node) {
  OnModelChanged({node.id(), scada::GetTypeDefinitionId(node),
                  scada::ModelChangeEvent::NodeAdded});
}

void AddressSpaceNodeService::OnNodeDeleted(const scada::Node& node) {
  OnModelChanged({node.id(), scada::GetTypeDefinitionId(node),
                  scada::ModelChangeEvent::NodeDeleted});
}

void AddressSpaceNodeService::OnNodeModified(
    const scada::Node& node,
    const scada::PropertyIds& property_ids) {
  const auto node_id = node.id();

  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id);

  if (auto i = nodes_.find(node_id); i != nodes_.end()) {
    if (auto ptr = i->second.lock())
      ptr->OnNodeSemanticChanged();
  }
}

void AddressSpaceNodeService::OnReferenceAdded(
    const scada::ReferenceType& reference_type,
    const scada::Node& source,
    const scada::Node& target) {
  scada::ModelChangeEvent event{source.id(), scada::GetTypeDefinitionId(source),
                                scada::ModelChangeEvent::ReferenceAdded};
  OnModelChanged(event);
  if (&source != &target)
    OnModelChanged(event);
}

void AddressSpaceNodeService::OnReferenceDeleted(
    const scada::ReferenceType& reference_type,
    const scada::Node& source,
    const scada::Node& target) {
  scada::ModelChangeEvent event{source.id(), scada::GetTypeDefinitionId(source),
                                scada::ModelChangeEvent::ReferenceDeleted};
  OnModelChanged(event);
  if (&source != &target)
    OnModelChanged(event);
}

NodeRef AddressSpaceNodeService::GetRemoteNode(const scada::Node* node) {
  if (!node)
    return nullptr;

  const auto node_id = node->id();

  auto& weak_model = nodes_[node_id];
  if (auto model = weak_model.lock())
    return model;

  auto& delegate = *static_cast<AddressSpaceNodeModelDelegate*>(this);
  auto model = std::make_shared<AddressSpaceNodeModel>(delegate, node_id);

  const auto fetch_status = node_fetch_status_checker_(node_id);
  model->OnNodeFetchStatusChanged(node, fetch_status);

  weak_model = model;
  return model;
}

void AddressSpaceNodeService::OnRemoteNodeModelDeleted(
    const scada::NodeId& node_id) {
  nodes_.erase(node_id);
}

void AddressSpaceNodeService::OnNodeFetchStatusChanged(
    const scada::NodeId& node_id,
    const NodeFetchStatus& status) {
  assert(node_fetch_status_checker_(node_id) == status);

  if (auto i = nodes_.find(node_id); i != nodes_.end()) {
    if (auto model = i->second.lock()) {
      auto* node = address_space_.GetNode(node_id);
      model->OnNodeFetchStatusChanged(node, status);
    }
  }
}
