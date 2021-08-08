#include "node_service/v1/address_space_fetcher_impl.h"

#include "address_space/address_space_impl.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "model/node_id_util.h"
#include "node_service/v1/address_space_updater.h"

#include "base/debug_util-inl.h"

namespace v1 {

namespace {

void GetAllNodeIdsHelper(const scada::Node& parent,
                         std::vector<scada::NodeId>& result) {
  result.emplace_back(parent.id());

  for (auto& ref : parent.forward_references()) {
    if (!scada::IsTypeDefinition(ref.node->GetNodeClass()) &&
        scada::IsSubtypeOf(*ref.type, scada::id::HierarchicalReferences) &&
        !scada::IsSubtypeOf(*ref.type, scada::id::HasProperty)) {
      GetAllNodeIdsHelper(*ref.node, result);
    }
  }
}

std::vector<scada::NodeId> GetAllNodeIds(scada::AddressSpace& address_space) {
  std::vector<scada::NodeId> result;
  if (auto* parent = address_space.GetNode(scada::id::RootFolder)) {
    result.reserve(32);
    GetAllNodeIdsHelper(*parent, result);
  }
  return result;
}

}  // namespace

// AddressSpaceFetcherImpl

AddressSpaceFetcherImpl::AddressSpaceFetcherImpl(
    AddressSpaceFetcherImplContext&& context)
    : AddressSpaceFetcherImplContext{std::move(context)},
      node_fetch_status_tracker_{{node_fetch_status_changed_handler_,
                                  [this](const scada::NodeId& node_id) {
                                    node_fetcher_->Fetch(node_id, true);
                                    return false;
                                  },
                                  address_space_}} {}

AddressSpaceFetcherImpl::~AddressSpaceFetcherImpl() {}

// static
std::shared_ptr<AddressSpaceFetcherImpl> AddressSpaceFetcherImpl::Create(
    AddressSpaceFetcherImplContext&& context) {
  auto fetcher = std::make_shared<AddressSpaceFetcherImpl>(std::move(context));
  fetcher->Init();
  return fetcher;
}

void AddressSpaceFetcherImpl::Init() {
  FetchCompletedHandler fetch_completed_handler =
      [weak_ptr = weak_from_this()](
          std::vector<scada::NodeState>&& fetched_nodes,
          NodeFetchStatuses&& errors) {
        if (auto ptr = weak_ptr.lock())
          ptr->OnFetchCompleted(std::move(fetched_nodes), std::move(errors));
      };

  NodeValidator node_validator = [this](const scada::NodeId& node_id) {
    return address_space_.GetNode(node_id) != 0;
  };

  node_fetcher_ = NodeFetcherImpl::Create(NodeFetcherImplContext{
      executor_, view_service_, attribute_service_,
      std::move(fetch_completed_handler), std::move(node_validator),
      std::make_shared<const scada::ServiceContext>()});

  ReferenceValidator reference_validator =
      [this](const scada::NodeId& node_id, scada::BrowseResult&& result) {
        OnChildrenFetched(node_id, std::move(result.references));
      };

  node_children_fetcher_ = NodeChildrenFetcher::Create({
      executor_,
      view_service_,
      reference_validator,
  });
}

void AddressSpaceFetcherImpl::OnChannelOpened() {
  channel_opened_ = true;

  for (const auto& node_id : GetAllNodeIds(address_space_)) {
    node_fetcher_->Fetch(node_id, true);
    node_children_fetcher_->Fetch(node_id);
  }

  PostponedFetchNodes postponed_fetch_nodes;
  postponed_fetch_nodes_.swap(postponed_fetch_nodes);
  for (auto& [node_id, requested_status] : postponed_fetch_nodes) {
    if (requested_status.node_fetched)
      node_fetcher_->Fetch(node_id, true);
    if (requested_status.children_fetched)
      node_children_fetcher_->Fetch(node_id);
  }
}

void AddressSpaceFetcherImpl::OnChannelClosed() {
  channel_opened_ = false;
}

void AddressSpaceFetcherImpl::DeleteNode(const scada::NodeId& node_id) {
  node_fetcher_->Cancel(node_id);
  node_children_fetcher_->Cancel(node_id);
  node_fetch_status_tracker_.Delete(node_id);
  address_space_.RemoveNode(node_id);
}

void AddressSpaceFetcherImpl::OnModelChanged(
    const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    LOG_INFO(logger_) << "Node deleted"
                      << LOG_TAG("NodeId", NodeIdToScadaString(event.node_id));

    DeleteNode(event.node_id);

  } else {
    if (event.verb & scada::ModelChangeEvent::NodeAdded) {
      LOG_INFO(logger_) << "Node added"
                        << LOG_TAG("NodeId",
                                   NodeIdToScadaString(event.node_id));
    }

    if (event.verb & (scada::ModelChangeEvent::ReferenceAdded |
                      scada::ModelChangeEvent::ReferenceDeleted)) {
      LOG_INFO(logger_)
          << "Reference changed"
          << LOG_TAG("NodeId", NodeIdToScadaString(event.node_id))
          << LOG_TAG("Added",
                     !!(event.verb & scada::ModelChangeEvent::ReferenceAdded))
          << LOG_TAG("Deleted", !!(event.verb &
                                   scada::ModelChangeEvent::ReferenceDeleted));

      if (address_space_.GetNode(event.node_id)) {
        LOG_INFO(logger_) << "Fetch references"
                          << LOG_TAG("NodeId",
                                     NodeIdToScadaString(event.node_id));

        // Fetch forward references.
        node_fetcher_->Fetch(event.node_id, true);

        // Fetch child references.
        // TODO: This can be avoided if no children were requested.
        node_children_fetcher_->Fetch(event.node_id);
      }
    }
  }

  model_changed_handler_(event);
}

void AddressSpaceFetcherImpl::OnNodeSemanticsChanged(
    const scada::SemanticChangeEvent& event) {
  LOG_INFO(logger_) << "NodeSemanticsChanged"
                    << LOG_TAG("NodeId", NodeIdToScadaString(event.node_id));

  if (address_space_.GetNode(event.node_id))
    node_fetcher_->Fetch(event.node_id, true);
}

void AddressSpaceFetcherImpl::FillMissingParent(scada::NodeState& node_state) {
  if (!node_state.parent_id.is_null() && !node_state.supertype_id.is_null())
    return;

  auto* node = address_space_.GetNode(node_state.node_id);
  if (!node)
    return;

  auto parent_reference = scada::GetParentReference(*node);
  if (!parent_reference)
    return;

  if (scada::IsSubtypeOf(*parent_reference.type, scada::id::HasSubtype)) {
    node_state.supertype_id = parent_reference.node->id();
  } else {
    node_state.parent_id = parent_reference.node->id();
    node_state.reference_type_id = parent_reference.type->id();
  }
}

void AddressSpaceFetcherImpl::OnFetchCompleted(
    std::vector<scada::NodeState>&& fetched_nodes,
    NodeFetchStatuses&& errors) {
  LOG_INFO(logger_) << "Nodes fetched"
                    << LOG_TAG("Count", fetched_nodes.size());
  LOG_DEBUG(logger_) << "Nodes fetched"
                     << LOG_TAG("Count", fetched_nodes.size())
                     << LOG_TAG("Nodes", ToString(fetched_nodes));

  if (!errors.empty()) {
    LOG_WARNING(logger_) << "Nodes fetch errors"
                         << LOG_TAG("Count", errors.size());
    LOG_DEBUG(logger_) << "Nodes fetch errors"
                       << LOG_TAG("Count", errors.size())
                       << LOG_TAG("Errors", ToString(errors));
  }

  for (auto& node_state : fetched_nodes)
    FillMissingParent(node_state);

  AddressSpaceUpdater updater{address_space_, node_factory_};
  updater.UpdateNodes(std::move(fetched_nodes));

  for (auto& [node_id, verb] : updater.model_change_verbs()) {
    if (verb & scada::ModelChangeEvent::NodeAdded)
      errors.emplace_back(node_id, scada::StatusCode::Good);
  }

  if (!errors.empty())
    node_fetch_status_tracker_.OnNodesFetched(errors);

  for (auto& [node_id, verb] : updater.model_change_verbs()) {
    if (auto* node = address_space_.GetNode(node_id)) {
      model_changed_handler_(scada::ModelChangeEvent{
          node->id(), scada::GetTypeDefinitionId(*node), verb});
    }
  }

  for (auto& node_id : updater.semantic_change_node_ids()) {
    if (auto* node = address_space_.GetNode(node_id))
      semantic_changed_handler_(scada::SemanticChangeEvent{node->id()});
  }
}

void AddressSpaceFetcherImpl::OnChildrenFetched(
    const scada::NodeId& node_id,
    scada::ReferenceDescriptions&& references) {
  // Children can be fetched before the node.
  if (auto* node = address_space_.GetNode(node_id)) {
    auto deleted_children = FindDeletedChildren(*node, references);
    for (const auto* deleted_child : deleted_children) {
      const auto deleted_child_id = deleted_child->id();
      const auto deleted_child_type_definition_id =
          scada::GetTypeDefinitionId(*deleted_child);

      LOG_INFO(logger_) << "Child node was deleted"
                        << LOG_TAG("ParentId", NodeIdToScadaString(node_id))
                        << LOG_TAG("ChildId",
                                   NodeIdToScadaString(deleted_child_id));
      DeleteNode(deleted_child_id);
      model_changed_handler_(scada::ModelChangeEvent{
          deleted_child_id, deleted_child_type_definition_id,
          scada::ModelChangeEvent::NodeDeleted});
    }
  }

  for (auto& reference : references) {
    assert(reference.forward);
    // The node could be fetched via non-hierarchical reference with no
    // children.
    if (!address_space_.GetNode(reference.node_id))
      node_fetcher_->Fetch(reference.node_id);
  }

  node_fetch_status_tracker_.OnChildrenFetched(node_id, std::move(references));
}

NodeChildrenFetcherContext
AddressSpaceFetcherImpl::MakeNodeChildrenFetcherContext() {
  ReferenceValidator reference_validator =
      [this](const scada::NodeId& node_id, scada::BrowseResult&& result) {
        OnChildrenFetched(node_id, std::move(result.references));
      };

  return {
      executor_,
      view_service_,
      reference_validator,
  };
}

std::pair<scada::Status, NodeFetchStatus>
AddressSpaceFetcherImpl::GetNodeFetchStatus(
    const scada::NodeId& node_id) const {
  return node_fetch_status_tracker_.GetStatus(node_id);
}

void AddressSpaceFetcherImpl::FetchNode(
    const scada::NodeId& node_id,
    const NodeFetchStatus& requested_status) {
  if (!channel_opened_) {
    postponed_fetch_nodes_[node_id] |= requested_status;
    return;
  }

  InternalFetchNode(node_id, requested_status);
}

void AddressSpaceFetcherImpl::InternalFetchNode(
    const scada::NodeId& node_id,
    const NodeFetchStatus& requested_status) {
  assert(channel_opened_);

  auto [status, fetch_status] = node_fetch_status_tracker_.GetStatus(node_id);

  if (fetch_status.node_fetched < requested_status.node_fetched)
    node_fetcher_->Fetch(node_id, true);

  if (fetch_status.children_fetched < requested_status.children_fetched)
    node_children_fetcher_->Fetch(node_id);
}

size_t AddressSpaceFetcherImpl::GetPendingTaskCount() const {
  return node_fetcher_->GetPendingNodeCount() +
         node_children_fetcher_->GetPendingNodeCount();
}

}  // namespace v1
