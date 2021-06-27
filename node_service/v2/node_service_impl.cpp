#include "node_service/v2/node_service_impl.h"

#include "address_space/address_space_util.h"
#include "core/attribute_service.h"
#include "core/standard_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_observer.h"
#include "node_service/node_util.h"
#include "node_service/v2/node_model_impl.h"

#include "base/debug_util-inl.h"

namespace v2 {

NodeServiceImpl::NodeServiceImpl(NodeServiceImplContext&& context)
    : NodeServiceImplContext(std::move(context)),
      node_fetcher_{NodeFetcherImpl::Create(MakeNodeFetcherImplContext())},
      node_children_fetcher_{
          NodeChildrenFetcher::Create(MakeNodeChildrenFetcherContext())} {}

NodeServiceImpl::~NodeServiceImpl() {}

NodeRef NodeServiceImpl::GetNode(const scada::NodeId& node_id) {
  auto node = GetNodeModel(node_id);
  if (!node)
    return nullptr;

  node->Fetch(NodeFetchStatus::NodeOnly(), {});
  return node;
}

std::shared_ptr<NodeModelImpl> NodeServiceImpl::GetNodeModel(
    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  auto& node = nodes_[node_id];
  if (!node)
    node = std::make_shared<NodeModelImpl>(*this, node_id);

  return node;
}

void NodeServiceImpl::Subscribe(NodeRefObserver& observer) const {
  assert(!observers_.HasObserver(&observer));
  observers_.AddObserver(&observer);
}

void NodeServiceImpl::Unsubscribe(NodeRefObserver& observer) const {
  observers_.RemoveObserver(&observer);
}

void NodeServiceImpl::NotifyModelChanged(const scada::ModelChangeEvent& event) {
  for (auto& o : observers_)
    o.OnModelChanged(event);
}

void NodeServiceImpl::NotifySemanticsChanged(const scada::NodeId& node_id) {
  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id);
}

void NodeServiceImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    if (auto i = nodes_.find(event.node_id); i != nodes_.end()) {
      /*auto model = std::move(i->second);
      nodes_.erase(i);
      model->OnModelChanged(
          {event.node_id, {}, scada::ModelChangeEvent::NodeDeleted});*/
    }
    return;
  }

  if (event.verb & scada::ModelChangeEvent::NodeAdded) {
    if (nodes_.find(event.node_id) != nodes_.end()) {
      // TODO: Log error.
      return;
    }
    return;
  }

  if (auto i = nodes_.find(event.node_id); i != nodes_.end())
    i->second->OnModelChanged(event);
}

void NodeServiceImpl::OnNodeSemanticsChanged(
    const scada::SemanticChangeEvent& event) {
  LOG_INFO(logger_) << "Node semantics changed"
                    << LOG_TAG("NodeId", NodeIdToScadaString(event.node_id));

  auto node = GetNodeModel(event.node_id);
  if (!node)
    return;

  NodeFetcher::ParentInfo parent_info{node->node_state_.parent_id,
                                      node->node_state_.reference_type_id,
                                      node->node_state_.supertype_id};
  node_fetcher_->Fetch(event.node_id, false, std::move(parent_info), true);
}

void NodeServiceImpl::OnFetchNode(const scada::NodeId& node_id,
                                  const NodeFetchStatus& requested_status) {
  if (!channel_opened_) {
    pending_fetch_nodes_[node_id] |= requested_status;
    return;
  }

  LOG_INFO(logger_) << "Fetch node"
                    << LOG_TAG("NodeId", NodeIdToScadaString(node_id))
                    << LOG_TAG("RequestedStatus", ToString(requested_status));

  if (requested_status.node_fetched)
    node_fetcher_->Fetch(node_id);
  if (requested_status.children_fetched)
    node_children_fetcher_->Fetch(node_id);
}

void NodeServiceImpl::ProcessFetchedNodes(
    std::vector<scada::NodeState>&& node_states) {
  // Must update all nodes at first, then notify all together.
  // TODO: Simplify

  SortNodesHierarchically(node_states);

  for (auto& node_state : node_states) {
    if (auto node = GetNodeModel(node_state.node_id))
      node->OnFetched(node_state);
  }

  for (auto& node_state : node_states) {
    if (auto node = GetNodeModel(node_state.node_id))
      node->OnFetchCompleted();
  }
}

void NodeServiceImpl::ProcessFetchErrors(NodeFetchStatuses&& errors) {
  for (auto& [node_id, status] : errors) {
    auto& model = nodes_[node_id];
    model->OnFetchError(std::move(status));
  }
}

NodeFetcherImplContext NodeServiceImpl::MakeNodeFetcherImplContext() {
  auto fetch_completed_handler =
      [this](std::vector<scada::NodeState>&& node_states,
             NodeFetchStatuses&& errors) {
        ProcessFetchedNodes(std::move(node_states));
        ProcessFetchErrors(std::move(errors));
      };

  NodeValidator node_validator = [this](const scada::NodeId& node_id) -> bool {
    auto i = nodes_.find(node_id);
    // TODO: Optimize |GetFetchStatus()|.
    return i != nodes_.end() && i->second->GetFetchStatus().node_fetched;
  };

  return {
      executor_,          view_service_,
      attribute_service_, fetch_completed_handler,
      node_validator,
  };
}

NodeChildrenFetcherContext NodeServiceImpl::MakeNodeChildrenFetcherContext() {
  ReferenceValidator reference_validator =
      [this](const scada::NodeId& node_id, scada::BrowseResult&& result) {
        if (auto node = GetNodeModel(node_id))
          node->OnChildrenFetched(std::move(result.references));
      };

  return {executor_, view_service_, reference_validator};
}

void NodeServiceImpl::OnChannelOpened() {
  LOG_INFO(logger_) << "Channel opened";

  assert(!channel_opened_);
  channel_opened_ = true;

  auto pending_fetch_nodes = std::move(pending_fetch_nodes_);
  pending_fetch_nodes_.clear();

  for (auto& [node_id, requested_status] : pending_fetch_nodes) {
    LOG_INFO(logger_) << "Fetch node"
                      << LOG_TAG("NodeId", NodeIdToScadaString(node_id))
                      << LOG_TAG("RequestedStatus", ToString(requested_status));

    if (requested_status.node_fetched)
      node_fetcher_->Fetch(node_id);
    if (requested_status.children_fetched)
      node_children_fetcher_->Fetch(node_id);
  }
}

void NodeServiceImpl::OnChannelClosed() {
  LOG_INFO(logger_) << "Channel closed";

  assert(channel_opened_);
  assert(pending_fetch_nodes_.empty());

  channel_opened_ = false;
}

size_t NodeServiceImpl::GetPendingTaskCount() const {
  return pending_fetch_nodes_.size() + node_fetcher_->GetPendingNodeCount() +
         node_children_fetcher_->GetPendingNodeCount();
}

}  // namespace v2
