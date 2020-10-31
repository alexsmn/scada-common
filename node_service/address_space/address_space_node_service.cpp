#include "node_service/address_space/address_space_node_service.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "core/attribute_service.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"
#include "node_service/address_space/address_space_node_model.h"
#include "node_service/node_observer.h"

#include "core/debug_util-inl.h"

namespace {

const scada::Node* GetSemanticParent(const scada::Node& node) {
  auto [reference_type, parent] = scada::GetParentReference(node);
  if (!reference_type)
    return nullptr;
  if (!scada::IsSubtypeOf(*reference_type, scada::id::HasProperty))
    return nullptr;
  return parent;
}

}  // namespace

// AddressSpaceNodeService

AddressSpaceNodeService::AddressSpaceNodeService(
    AddressSpaceNodeServiceContext&& context)
    : AddressSpaceNodeServiceContext{std::move(context)},
      fetcher_{AddressSpaceFetcher::Create(MakeAddressSpaceFetcherContext())} {
  address_space_.Subscribe(*this);
}

AddressSpaceNodeService::~AddressSpaceNodeService() {
  assert(!observers_.might_have_observers());

  address_space_.Unsubscribe(*this);

  nodes_.clear();
}

NodeRef AddressSpaceNodeService::GetNode(const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  auto& model = nodes_[node_id];
  if (model)
    return model;

  model = std::make_shared<AddressSpaceNodeModel>(AddressSpaceNodeModelContext{
      *this,
      node_id,
      attribute_service_,
      monitored_item_service_,
      method_service_,
  });

  auto* node = address_space_.GetNode(node_id);
  auto [status, fetch_status] = fetcher_->GetNodeFetchStatus(node_id);
  model->SetFetchStatus(node, std::move(status), fetch_status);

  if (!fetch_status.node_fetched)
    fetcher_->FetchNode(node_id, NodeFetchStatus::NodeOnly());

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
  // Model must handle node deletion first to correctly cleanup a refrence to
  // the node, since it has already been deleted.
  if (auto i = nodes_.find(event.node_id); i != nodes_.end())
    i->second->OnModelChanged(event);

  for (auto& o : observers_)
    o.OnModelChanged(event);
}

void AddressSpaceNodeService::OnNodeCreated(const scada::Node& node) {}

void AddressSpaceNodeService::OnNodeDeleted(const scada::Node& node) {
  if (auto i = nodes_.find(node.id()); i != nodes_.end())
    i->second->OnNodeDeleted();
}

void AddressSpaceNodeService::OnNodeModified(
    const scada::Node& node,
    const scada::PropertyIds& property_ids) {
  auto* semantic_node = &node;
  while (auto* n = GetSemanticParent(*semantic_node))
    semantic_node = n;

  const auto& node_id = semantic_node->id();

  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id);

  if (auto i = nodes_.find(node_id); i != nodes_.end())
    i->second->OnNodeSemanticChanged();
}

void AddressSpaceNodeService::OnReferenceAdded(
    const scada::ReferenceType& reference_type,
    const scada::Node& source,
    const scada::Node& target) {
  OnModelChanged({source.id(), scada::GetTypeDefinitionId(source),
                  scada::ModelChangeEvent::ReferenceAdded});
  if (&source != &target) {
    OnModelChanged({target.id(), scada::GetTypeDefinitionId(target),
                    scada::ModelChangeEvent::ReferenceAdded});
  }
}

void AddressSpaceNodeService::OnReferenceDeleted(
    const scada::ReferenceType& reference_type,
    const scada::Node& source,
    const scada::Node& target) {
  OnModelChanged({source.id(), scada::GetTypeDefinitionId(source),
                  scada::ModelChangeEvent::ReferenceDeleted});
  if (&source != &target) {
    OnModelChanged({target.id(), scada::GetTypeDefinitionId(target),
                    scada::ModelChangeEvent::ReferenceDeleted});
  }
}

NodeRef AddressSpaceNodeService::GetRemoteNode(const scada::Node* node) {
  return node ? GetNode(node->id()) : nullptr;
}

void AddressSpaceNodeService::OnNodeModelDeleted(const scada::NodeId& node_id) {
  nodes_.erase(node_id);
}

void AddressSpaceNodeService::OnNodeModelFetchRequested(
    const scada::NodeId& node_id,
    const NodeFetchStatus& requested_status) {
  fetcher_->FetchNode(node_id, requested_status);
}

void AddressSpaceNodeService::OnNodeFetchStatusChanged(
    const scada::NodeId& node_id,
    const scada::Status& status,
    const NodeFetchStatus& fetch_status) {
  assert(fetcher_->GetNodeFetchStatus(node_id).first.code() == status.code());
  assert(fetcher_->GetNodeFetchStatus(node_id).second == fetch_status);

  auto* node = address_space_.GetNode(node_id);

  if (auto i = nodes_.find(node_id); i != nodes_.end())
    i->second->SetFetchStatus(node, status, fetch_status);

  const scada::ModelChangeEvent event{
      node_id, node ? scada::GetTypeDefinitionId(*node) : scada::NodeId{},
      scada::ModelChangeEvent::ReferenceAdded |
          scada::ModelChangeEvent::ReferenceDeleted};
  for (auto& o : observers_) {
    o.OnNodeFetched(node_id, fetch_status.children_fetched);
    o.OnModelChanged(event);
    o.OnNodeSemanticChanged(node_id);
  }
}

AddressSpaceFetcherContext
AddressSpaceNodeService::MakeAddressSpaceFetcherContext() {
  auto node_fetch_status_changed_handler =
      [this](const scada::NodeId& node_id, const scada::Status& status,
             const NodeFetchStatus& fetch_status) {
        OnNodeFetchStatusChanged(node_id, status, fetch_status);
      };

  auto model_changed_handler = [this](const scada::ModelChangeEvent& event) {
    OnModelChanged(event);
  };

  return {io_context_,
          executor_,
          view_service_,
          attribute_service_,
          monitored_item_service_,
          address_space_,
          node_factory_,
          node_fetch_status_changed_handler,
          model_changed_handler};
}

void AddressSpaceNodeService::OnChannelOpened() {
  fetcher_->OnChannelOpened();
}

void AddressSpaceNodeService::OnChannelClosed() {
  fetcher_->OnChannelClosed();
}
