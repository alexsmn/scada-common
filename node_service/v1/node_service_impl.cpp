#include "node_service/v1/node_service_impl.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "base/check.h"
#include "node_service/v1/address_space_fetcher.h"
#include "node_service/v1/node_fetch_status_types.h"
#include "node_service/v1/node_model_impl.h"
#include "scada/event.h"

#include "base/debug_util.h"

#include <format>

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
      address_space_fetcher_{address_space_fetcher_factory_(
          MakeAddressSpaceFetcherFactoryContext())},
      node_deleted_connection_{address_space_.SubscribeNodeDeleted(
          [this](const scada::Node& node) { OnNodeDeleted(node); })} {}

NodeServiceImpl::~NodeServiceImpl() {
  node_deleted_connection_.disconnect();

  nodes_.clear();
}

NodeRef NodeServiceImpl::GetNode(const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  auto& model = nodes_[node_id];
  if (model)
    return model;

  model = std::make_shared<NodeModelImpl>(
      NodeModelImplContext{.delegate_ = *this,
                           .node_id_ = node_id,
                           .scada_node_ = scada_client_.node(node_id)});

  auto* node = address_space_.GetNode(node_id);
  auto [status, fetch_status] =
      address_space_fetcher_->GetNodeFetchStatus(node_id);
  model->SetFetchStatus(node, std::move(status), fetch_status);

  return model;
}

boost::signals2::scoped_connection NodeServiceImpl::SubscribeModelChanged(
    const ModelChangedCallback& callback) const {
  return signals_.model_changed.connect(callback);
}

boost::signals2::scoped_connection
NodeServiceImpl::SubscribeNodeSemanticChanged(
    const NodeSemanticChangedCallback& callback) const {
  return signals_.node_semantic_changed.connect(callback);
}

boost::signals2::scoped_connection NodeServiceImpl::SubscribeNodeFetched(
    const NodeFetchedCallback& callback) const {
  return signals_.node_fetched.connect(callback);
}

boost::signals2::scoped_connection NodeServiceImpl::SubscribeNodeStateChanged(
    const NodeStateChangedCallback& callback) const {
  return signals_.node_state_changed.connect(callback);
}

void NodeServiceImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  LOG_INFO(logger_) << "Model changed"
                    << LOG_TAG("Event", std::format("{}", event));

  // Model must handle node deletion first to correctly cleanup a refrence to
  // the node, since it has already been deleted.
  if (auto i = nodes_.find(event.node_id); i != nodes_.end())
    i->second->OnModelChanged(event);

  signals_.model_changed(event);
}

void NodeServiceImpl::OnSemanticChanged(
    const scada::SemanticChangeEvent& event) {
  LOG_INFO(logger_) << "Semantic changed"
                    << LOG_TAG("Event", std::format("{}", event));

  signals_.node_semantic_changed(event.node_id);

  if (auto i = nodes_.find(event.node_id); i != nodes_.end())
    i->second->OnNodeSemanticChanged();
}

void NodeServiceImpl::OnNodeDeleted(const scada::Node& node) {
  if (auto i = nodes_.find(node.id()); i != nodes_.end())
    i->second->OnNodeDeleted();
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
  address_space_fetcher_->FetchNode(node_id, requested_status);
}

void NodeServiceImpl::OnNodeFetchStatusChanged(
    std::span<const NodeFetchStatusChangedItem> items) {
  LOG_INFO(logger_) << "Node fetch status changed"
                    << LOG_TAG("items.size", items.size());
  LOG_DEBUG(logger_) << "Node fetch status changed"
                     << LOG_TAG("items.size", items.size())
                     << LOG_TAG("items", ToString(items));

  // First: update node impl statuses.
  for (const auto& [node_id, status, fetch_status] : items) {
    base::Check(
        address_space_fetcher_->GetNodeFetchStatus(node_id).first.code() ==
        status.code());
    base::Check(address_space_fetcher_->GetNodeFetchStatus(node_id).second ==
                fetch_status);

    auto* node = address_space_.GetNode(node_id);

    if (auto i = nodes_.find(node_id); i != nodes_.end())
      i->second->SetFetchStatus(node, status, fetch_status);
  }

  // Second: report all statuses.
  for (const auto& [node_id, status, fetch_status] : items) {
    if (auto i = nodes_.find(node_id); i != nodes_.end())
      i->second->NotifyFetchStatus();

    signals_.node_fetched({node_id});
    signals_.node_semantic_changed(node_id);
  }
}

AddressSpaceFetcherFactoryContext
NodeServiceImpl::MakeAddressSpaceFetcherFactoryContext() {
  // There is not need ot use |BindExecutor()|. It's important to process these
  // events synchronously. Also |BindExecutor()| will capture all passed
  // arguments, be careful about |span|.

  NodeFetchStatusChangedHandler node_fetch_status_changed_handler =
      [this](std::span<const NodeFetchStatusChangedItem> items) {
        OnNodeFetchStatusChanged(items);
      };

  auto model_changed_handler = [this](const scada::ModelChangeEvent& event) {
    OnModelChanged(event);
  };

  auto semantic_changed_handler =
      [this](const scada::SemanticChangeEvent& event) {
        OnSemanticChanged(event);
      };

  return AddressSpaceFetcherFactoryContext{
      std::move(node_fetch_status_changed_handler),
      std::move(model_changed_handler),
      std::move(semantic_changed_handler),
  };
}

void NodeServiceImpl::OnChannelOpened() {
  address_space_fetcher_->OnChannelOpened();
}

void NodeServiceImpl::OnChannelClosed() {
  address_space_fetcher_->OnChannelClosed();
}

size_t NodeServiceImpl::GetPendingTaskCount() const {
  return address_space_fetcher_->GetPendingTaskCount();
}

}  // namespace v1
