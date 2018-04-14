#include "common/address_space_fetcher.h"

#include "base/logger.h"
#include "common/address_space_updater.h"
#include "common/node_id_util.h"
#include "core/configuration_impl.h"
#include "core/node_utils.h"
#include "core/standard_node_ids.h"
#include "core/type_definition.h"

namespace {

template <class Key, class Value>
std::set<Key> MakeKeySet(const std::map<Key, Value>& map) {
  std::set<Key> set;
  for (auto& p : map)
    set.emplace(p.first);
  return set;
}

void GetAllNodeIdsHelper(scada::Node& parent,
                         std::vector<scada::NodeId>& result) {
  result.emplace_back(parent.id());

  for (auto& ref : parent.forward_references()) {
    if (scada::IsSubtypeOf(*ref.type, scada::id::HierarchicalReferences) &&
        !scada::IsSubtypeOf(*ref.type, scada::id::HasProperty)) {
      GetAllNodeIdsHelper(*ref.node, result);
    }
  }
}

std::vector<scada::NodeId> GetAllNodeIds(scada::Configuration& address_space) {
  std::vector<scada::NodeId> result;
  if (auto* parent = address_space.GetNode(scada::id::RootFolder)) {
    result.reserve(32);
    GetAllNodeIdsHelper(*parent, result);
  }
  return result;
}

}  // namespace

// AddressSpaceFetcher

AddressSpaceFetcher::AddressSpaceFetcher(AddressSpaceFetcherContext&& context)
    : AddressSpaceFetcherContext{std::move(context)},
      node_fetcher_{MakeNodeFetcherContext()},
      node_children_fetcher_{MakeNodeChildrenFetcherContext()},
      fetch_status_tracker_{
          {node_fetch_status_changed_handler_, address_space_}} {
  view_service_.Subscribe(*this);
}

AddressSpaceFetcher::~AddressSpaceFetcher() {
  view_service_.Unsubscribe(*this);
}

void AddressSpaceFetcher::OnChannelOpened() {
  channel_opened_ = true;

  if (!smart_fetch_) {
    for (const auto& node_id : GetAllNodeIds(address_space_)) {
      node_fetcher_.Fetch(node_id);
      node_children_fetcher_.Fetch(node_id);
    }
  }

  PostponedFetchNodes postponed_fetch_nodes;
  postponed_fetch_nodes_.swap(postponed_fetch_nodes);
  for (auto& [node_id, requested_status] : postponed_fetch_nodes)
    InternalFetchNode(node_id, requested_status);
}

void AddressSpaceFetcher::OnChannelClosed() {
  channel_opened_ = false;
}

void AddressSpaceFetcher::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    logger_->WriteF(LogSeverity::Normal, "NodeDeleted [node_id=%s]",
                    NodeIdToScadaString(event.node_id).c_str());

    node_fetcher_.Cancel(event.node_id);
    node_children_fetcher_.Cancel(event.node_id);
    fetch_status_tracker_.Delete(event.node_id);
    address_space_.RemoveNode(event.node_id);

    return;
  }

  if (event.verb & scada::ModelChangeEvent::NodeAdded) {
    logger_->WriteF(LogSeverity::Normal, "NodeAdded [node_id=%s]",
                    NodeIdToScadaString(event.node_id).c_str());

    if (!smart_fetch_) {
      node_fetcher_.Fetch(event.node_id, true);
      node_children_fetcher_.Fetch(event.node_id);
    }
  }

  if (event.verb & (scada::ModelChangeEvent::ReferenceAdded |
                    scada::ModelChangeEvent::ReferenceDeleted)) {
    logger_->WriteF(LogSeverity::Normal, "ReferenceChanged [node_id=%s]",
                    NodeIdToScadaString(event.node_id).c_str());

    if (address_space_.GetNode(event.node_id)) {
      // Fetch forward references.
      node_fetcher_.Fetch(event.node_id);
      // Fetch child references.
      node_children_fetcher_.Fetch(event.node_id);
    }
  }
}

void AddressSpaceFetcher::OnNodeSemanticsChanged(const scada::NodeId& node_id) {
  logger_->WriteF(LogSeverity::Normal, "NodeSemanticsChanged [node_id=%s]",
                  NodeIdToScadaString(node_id).c_str());

  if (address_space_.GetNode(node_id))
    node_fetcher_.Fetch(node_id);
}

NodeFetcherContext AddressSpaceFetcher::MakeNodeFetcherContext() {
  auto fetch_completed_handler =
      [this](std::vector<scada::NodeState>&& fetched_nodes) {
        logger_->WriteF(LogSeverity::Normal, "%Iu nodes fetched",
                        fetched_nodes.size());

        std::vector<scada::Node*> added_nodes;
        UpdateNodes(address_space_, node_factory_, std::move(fetched_nodes),
                    *logger_, &added_nodes);

        for (auto* added_node : added_nodes) {
          const auto added_id = added_node->id();
          fetch_status_tracker_.OnNodeFetched(added_id);
          address_space_.NotifyNodeAdded(*added_node);
          if (!fetch_status_tracker_.GetStatus(added_id).children_fetched)
            node_children_fetcher_.Fetch(added_id);
        }
      };

  NodeValidator node_validator = [this](const scada::NodeId& node_id) {
    return address_space_.GetNode(node_id) != 0;
  };

  return {*logger_, view_service_, attribute_service_, fetch_completed_handler,
          node_validator};
}

NodeChildrenFetcherContext
AddressSpaceFetcher::MakeNodeChildrenFetcherContext() {
  ReferenceValidator reference_validator = [this](const scada::NodeId& node_id,
                                                  ReferenceMap references) {
    std::set<scada::NodeId> child_ids;
    std::vector<scada::Node*> missing_children;

    for (auto& [reference_type_id, children] : references) {
      missing_children.clear();
      FindMissingTargets(address_space_, node_id, reference_type_id,
                         MakeKeySet(children), missing_children);
      for (auto* missing_child : missing_children) {
        const auto missing_child_id = missing_child->id();
        logger_->WriteF(LogSeverity::Normal,
                        "Node %s: Missing target %s was deleted",
                        NodeIdToScadaString(node_id).c_str(),
                        NodeIdToScadaString(missing_child_id).c_str());
        fetch_status_tracker_.Delete(missing_child_id);
        address_space_.RemoveNode(missing_child_id);
      }

      for (auto& [child_id, child_reference_type_id] : children) {
        child_ids.insert(child_id);
        // The node could be fetched via non-hierarchical reference with no
        // children.
        if (!address_space_.GetNode(child_id)) {
          node_fetcher_.Fetch(child_id, false, node_id,
                              child_reference_type_id);
        }
        // Children are fetched from OnNodeFetched().
      }
    }

    fetch_status_tracker_.OnChildrenFetched(node_id, std::move(child_ids));
  };

  return {
      *logger_,
      view_service_,
      reference_validator,
  };
}

NodeFetchStatus AddressSpaceFetcher::GetNodeFetchStatus(
    const scada::NodeId& node_id) const {
  return fetch_status_tracker_.GetStatus(node_id);
}

void AddressSpaceFetcher::FetchNode(const scada::NodeId& node_id,
                                    const NodeFetchStatus& requested_status) {
  if (!smart_fetch_)
    return;

  if (!channel_opened_) {
    postponed_fetch_nodes_[node_id] |= requested_status;
    return;
  }

  InternalFetchNode(node_id, requested_status);
}

void AddressSpaceFetcher::InternalFetchNode(
    const scada::NodeId& node_id,
    const NodeFetchStatus& requested_status) {
  assert(smart_fetch_);
  assert(channel_opened_);

  auto status = fetch_status_tracker_.GetStatus(node_id);
  if (status.node_fetched < requested_status.node_fetched)
    node_fetcher_.Fetch(node_id, true);
  if (status.children_fetched < requested_status.children_fetched)
    node_children_fetcher_.Fetch(node_id);
}
