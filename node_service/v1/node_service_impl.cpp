#include "node_service/v1/node_service_impl.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "core/attribute_service.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"
#include "node_service/node_observer.h"
#include "node_service/v1/node_model_impl.h"

#include "base/debug_util-inl.h"

namespace v1 {

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

// NodeServiceImpl

NodeServiceImpl::NodeServiceImpl(NodeServiceImplContext&& context)
    : NodeServiceImplContext{std::move(context)},
      fetcher_{AddressSpaceFetcher::Create(MakeAddressSpaceFetcherContext())} {
  address_space_.Subscribe(*this);
}

NodeServiceImpl::~NodeServiceImpl() {
  assert(!observers_.might_have_observers());

  address_space_.Unsubscribe(*this);

  nodes_.clear();
}

NodeRef NodeServiceImpl::GetNode(const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  auto& model = nodes_[node_id];
  if (model)
    return model;

  model = std::make_shared<NodeModelImpl>(AddressSpaceNodeModelContext{
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

void NodeServiceImpl::Subscribe(NodeRefObserver& observer) const {
  observers_.AddObserver(&observer);
}

void NodeServiceImpl::Unsubscribe(NodeRefObserver& observer) const {
  observers_.RemoveObserver(&observer);
}

void NodeServiceImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  // Model must handle node deletion first to correctly cleanup a refrence to
  // the node, since it has already been deleted.
  if (auto i = nodes_.find(event.node_id); i != nodes_.end())
    i->second->OnModelChanged(event);

  for (auto& o : observers_)
    o.OnModelChanged(event);
}

void NodeServiceImpl::OnSemanticChanged(
    const scada::SemanticChangeEvent& event) {
  for (auto& o : observers_)
    o.OnNodeSemanticChanged(event.node_id);

  if (auto i = nodes_.find(event.node_id); i != nodes_.end())
    i->second->OnNodeSemanticChanged();
}

void NodeServiceImpl::OnNodeCreated(const scada::Node& node) {}

void NodeServiceImpl::OnNodeDeleted(const scada::Node& node) {
  if (auto i = nodes_.find(node.id()); i != nodes_.end())
    i->second->OnNodeDeleted();
}

void NodeServiceImpl::OnNodeModified(const scada::Node& node,
                                     const scada::PropertyIds& property_ids) {
  // This is handled in |OnSemanticChanged()|.
}

void NodeServiceImpl::OnReferenceAdded(
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

void NodeServiceImpl::OnReferenceDeleted(
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

NodeRef NodeServiceImpl::GetRemoteNode(const scada::Node* node) {
  return node ? GetNode(node->id()) : nullptr;
}

void NodeServiceImpl::OnNodeModelDeleted(const scada::NodeId& node_id) {
  nodes_.erase(node_id);
}

void NodeServiceImpl::OnNodeModelFetchRequested(
    const scada::NodeId& node_id,
    const NodeFetchStatus& requested_status) {
  fetcher_->FetchNode(node_id, requested_status);
}

void NodeServiceImpl::OnNodeFetchStatusChanged(
    base::span<const NodeFetchStatusChangedItem> items) {
  // First: update node impl statuses.
  for (const auto& [node_id, status, fetch_status] : items) {
    assert(fetcher_->GetNodeFetchStatus(node_id).first.code() == status.code());
    assert(fetcher_->GetNodeFetchStatus(node_id).second == fetch_status);

    auto* node = address_space_.GetNode(node_id);

    if (auto i = nodes_.find(node_id); i != nodes_.end())
      i->second->SetFetchStatus(node, status, fetch_status);
  }

  // Second: report all statuses.
  for (const auto& [node_id, status, fetch_status] : items) {
    if (auto i = nodes_.find(node_id); i != nodes_.end())
      i->second->NotifyFetchStatus();

    auto* node = address_space_.GetNode(node_id);
    const scada::ModelChangeEvent reference_added_deleted_event{
        node_id, node ? scada::GetTypeDefinitionId(*node) : scada::NodeId{},
        scada::ModelChangeEvent::ReferenceAdded |
            scada::ModelChangeEvent::ReferenceDeleted};

    for (auto& o : observers_) {
      o.OnNodeFetched(node_id, fetch_status.children_fetched);
      if (status)
        o.OnModelChanged(reference_added_deleted_event);
      o.OnNodeSemanticChanged(node_id);
    }
  }
}

AddressSpaceFetcherContext NodeServiceImpl::MakeAddressSpaceFetcherContext() {
  auto node_fetch_status_changed_handler =
      [this](base::span<const NodeFetchStatusChangedItem> items) {
        OnNodeFetchStatusChanged(items);
      };

  auto model_changed_handler = [this](const scada::ModelChangeEvent& event) {
    OnModelChanged(event);
  };

  auto semantic_changed_handler =
      [this](const scada::SemanticChangeEvent& event) {
        OnSemanticChanged(event);
      };

  /*auto view_events_provider =
      [&monitored_item_service =
           monitored_item_service_](scada::ViewEvents& events) {
        return std::make_unique<ViewEventsSubscription>(monitored_item_service,
                                                        events);
      };*/

  return {
      executor_,
      view_service_,
      attribute_service_,
      address_space_,
      node_factory_,
      view_events_provider_,
      node_fetch_status_changed_handler,
      model_changed_handler,
      semantic_changed_handler,
  };
}

void NodeServiceImpl::OnChannelOpened() {
  fetcher_->OnChannelOpened();
}

void NodeServiceImpl::OnChannelClosed() {
  fetcher_->OnChannelClosed();
}

size_t NodeServiceImpl::GetPendingTaskCount() const {
  return fetcher_->GetPendingTaskCount();
}

}  // namespace v1
