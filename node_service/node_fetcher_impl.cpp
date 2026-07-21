#include "node_service/node_fetcher_impl.h"

#include "base/auto_reset.h"
#include "base/awaitable.h"
#include "base/check.h"
#include "base/range_util.h"
#include "model/node_id_util.h"
#include "scada/attribute_ids.h"
#include "scada/attribute_service.h"
#include "scada/node_class.h"
#include "scada/standard_node_ids.h"
#include "scada/view_service.h"

#include "base/debug_util.h"

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/combine.hpp>

namespace {

const size_t kMaxRequestNodeCount = 100;
const size_t kMaxParallelRequestCount = 5;
// Read + Browse.
const size_t kPrimitiveRequestCount = 2;
const size_t kFetchAttributesReserveFactor = 5;
const size_t kFetchReferencesReserveFactor = 3;

void LogUnexpectedAttributeValue(BoostLogger& logger,
                                 const FetchingNode& node,
                                 scada::AttributeId attribute_id,
                                 const scada::Variant& value) {
  LOG_ERROR(logger) << "Unexpected fetched attribute type"
                    << LOG_TAG("NodeId",
                               NodeIdToScadaString(node.node_state.node_id))
                    << LOG_TAG("AttributeId", ToString(attribute_id))
                    << LOG_TAG("VariantType", ToString(value.type()))
                    << LOG_TAG("Value", ToString(value));
}

void GetFetchAttributes(const FetchingNode& fetching_node,
                        std::vector<scada::ReadValueId>& read_ids) {
  const auto& node_id = fetching_node.node_state.node_id;

  read_ids.emplace_back(node_id, scada::AttributeId::BrowseName);

  // Have to fetch diplay names for instance properties as well, since property
  // declaration might not be created.
  read_ids.emplace_back(node_id, scada::AttributeId::DisplayName);

  if (!fetching_node.is_property || fetching_node.is_declaration)
    read_ids.emplace_back(node_id, scada::AttributeId::NodeClass);

  read_ids.emplace_back(node_id, scada::AttributeId::DataType);

  // Must read only property values.
  // Must read values of type definition properties to correctly read
  // EnumStrings.
  if (fetching_node.is_property)
    read_ids.emplace_back(node_id, scada::AttributeId::Value);
}

void GetFetchReferences(const FetchingNode& fetching_node,
                        std::vector<scada::BrowseDescription>& descriptions) {
  const auto& node_id = fetching_node.node_state.node_id;

  // Don't request references of a property.
  if (!fetching_node.is_property) {
    descriptions.emplace_back(node_id, scada::BrowseDirection::Forward,
                              scada::id::Aggregates, true);
  }

  // Request property categories.
  if (!fetching_node.is_property || fetching_node.is_declaration) {
    descriptions.emplace_back(node_id, scada::BrowseDirection::Forward,
                              scada::id::NonHierarchicalReferences, true);
  }

  if (Includes(fetching_node.fetch_started,
               NodeFetchStatus::NonHierarchicalInverseReferences)) {
    descriptions.emplace_back(node_id, scada::BrowseDirection::Inverse,
                              scada::id::NonHierarchicalReferences, true);
  }

  // Request parent if unknown. It may happen when node is created.
  const bool fetch_parent =
      node_id != scada::id::RootFolder &&
      fetching_node.node_state.parent_id.is_null() &&
      fetching_node.node_state.reference_type_id.is_null();
  if (fetch_parent) {
    descriptions.emplace_back(node_id, scada::BrowseDirection::Inverse,
                              scada::id::HierarchicalReferences, true);
  }
}

std::vector<scada::NodeId> CollectNodeIds(
    const std::vector<FetchingNode*> nodes) {
  return nodes | boost::adaptors::transformed([](const FetchingNode* node) {
           return node->node_state.node_id;
         }) |
         to_vector;
}

std::vector<scada::NodeId> CollectFetchedNodeIds(
    const std::vector<scada::NodeState>& nodes) {
  return nodes | boost::adaptors::transformed([](const scada::NodeState& node) {
           return node.node_id;
         }) |
         to_vector;
}

}  // namespace

NodeFetcherImpl::NodeFetcherImpl(NodeFetcherImplContext&& context)
    : NodeFetcherImplContext{std::move(context)} {}

NodeFetcherImpl::~NodeFetcherImpl() = default;

// static
std::shared_ptr<NodeFetcherImpl> NodeFetcherImpl::Create(
    NodeFetcherImplContext&& context) {
  return std::shared_ptr<NodeFetcherImpl>(
      new NodeFetcherImpl(std::move(context)));
}

size_t NodeFetcherImpl::GetPendingNodeCount() const {
  return fetching_nodes_.size();
}

void NodeFetcherImpl::Fetch(const scada::NodeId& node_id,
                            NodeFetchStatus status,
                            bool force) {
  scada::base::Check(!node_id.is_null());
  scada::base::Check(Includes(status, NodeFetchStatus::NodeOnly));
  scada::base::Check(CheckInvariants());

  auto& node = fetching_nodes_.AddNode(node_id);

  FetchNode(node, next_pending_sequence_++, status, force);

  scada::base::Check(CheckInvariants());
}

void NodeFetcherImpl::FetchNode(FetchingNode& node,
                                unsigned pending_sequence,
                                NodeFetchStatus status,
                                bool force) {
  scada::base::Check(Includes(status, NodeFetchStatus::NodeOnly));

  if (!IsEmpty(node.fetch_started)) {
    if (Includes(node.fetch_started, status) && !force)
      return;

    LOG_INFO(logger_) << "Cancel started fetching node"
                      << LOG_TAG("NodeId", ToString(node.node_state.node_id))
                      << LOG_TAG("RequestId", node.fetch_request_id)
                      << LOG_TAG("FetchStarted", node.fetch_started);

    node.fetch_started = NodeFetchStatus::None;
    node.fetch_request_id = 0;
    node.attributes_fetched = false;
    node.references_fetched = false;
    node.status = scada::StatusCode::Good;
    // It's important to reset references, because they will be refetched.
    node.node_state.references.clear();
  }

  // LOG_INFO(logger_) << "Schedule fetch node"
  //                   << LOG_TAG("NodeId", ToString(node.node_id));

  node.force |= force;
  node.pending_status |= status;
  node.pending_sequence = pending_sequence;
  pending_queue_.push(node);

  FetchPendingNodes();
}

void NodeFetcherImpl::Cancel(const scada::NodeId& node_id) {
  scada::base::Check(CheckInvariants());

  auto* fetching_node = fetching_nodes_.FindNode(node_id);
  if (!fetching_node)
    return;

  LOG_INFO(logger_) << "Cancel fetching node"
                    << LOG_TAG("NodeId", ToString(node_id));

  pending_queue_.erase(*fetching_node);

  fetching_nodes_.RemoveNode(node_id);

  NotifyFetchedNodes();

  FetchPendingNodes();

  scada::base::Check(CheckInvariants());
}

void NodeFetcherImpl::FetchPendingNodes() {
  scada::base::Check(CheckInvariants());

  if (processing_response_)
    return;

  // Skip if a drain is already running higher on the stack. Handlers
  // that call Fetch() synchronously (in-process services) drop their
  // newly-queued nodes into |pending_queue_|; the outer loop below
  // picks them up without growing the stack.
  if (draining_pending_queue_)
    return;
  scada::base::AutoReset<bool> drain_guard{&draining_pending_queue_, true};

  while (!pending_queue_.empty() &&
         running_request_count_ + kPrimitiveRequestCount <=
             kMaxParallelRequestCount) {
    std::vector<FetchingNode*> nodes;
    nodes.reserve(std::min(kMaxRequestNodeCount, pending_queue_.size()));

    while (!pending_queue_.empty() && nodes.size() < kMaxRequestNodeCount) {
      auto& node = pending_queue_.top();
      pending_queue_.pop();
      scada::base::Check(IsEmpty(node.fetch_started));
      scada::base::Check(!IsEmpty(node.pending_status));
      node.fetch_started = node.pending_status;
      nodes.emplace_back(&node);
    }

    FetchPendingNodes(std::move(nodes));
  }

  scada::base::Check(CheckInvariants());
}

unsigned NodeFetcherImpl::MakeRequestId() {
  const auto request_id = next_request_id_++;
  if (next_request_id_ == 0)
    next_request_id_ = 1;
  return request_id;
}

void NodeFetcherImpl::FetchPendingNodes(std::vector<FetchingNode*>&& nodes) {
  scada::base::Check(CheckInvariants());

  const auto start_ticks = scada::base::TimeTicks::Now();
  const auto request_id = MakeRequestId();

  LOG_INFO(logger_) << "Fetch pending nodes"
                    << LOG_TAG("NodeIds", ToString(CollectNodeIds(nodes)))
                    << LOG_TAG("RequestId", request_id)
                    << LOG_TAG("RequestCount", running_request_count_);

  // Increment immediately for the case OnReadResult happens synchronously.
  running_request_count_ += kPrimitiveRequestCount;

  for (auto* node : nodes)
    node->fetch_request_id = request_id;

  // Attributes

  std::vector<scada::ReadValueId> read_ids;
  read_ids.reserve(nodes.size() * kFetchAttributesReserveFactor);
  for (auto* node : nodes) {
    size_t count = read_ids.size();
    GetFetchAttributes(*node, read_ids);
    node->attributes_fetched = count == read_ids.size();
  }

  scada::base::Check(CheckInvariants());

  CoSpawn(executor_, weak_from_this(),
          [request_id, start_ticks, read_ids = std::move(read_ids)](
              std::shared_ptr<NodeFetcherImpl> self) -> Awaitable<void> {
            // OnReadResult needs the inputs after the call, so pass a copy in.
            auto result = co_await self->attribute_service_.Read(
                self->service_context_, read_ids);
            auto status = result.status();
            auto results = std::move(result).value_or({});
            self->OnReadResult(request_id, start_ticks, std::move(status),
                               read_ids, std::move(results));
          });

  // References

  std::vector<scada::BrowseDescription> descriptions;
  descriptions.reserve(nodes.size() * kFetchReferencesReserveFactor);
  for (auto* node : nodes) {
    size_t count = descriptions.size();
    GetFetchReferences(*node, descriptions);
    node->references_fetched = count == descriptions.size();
  }

  if (descriptions.empty()) {
    OnBrowseResult(request_id, start_ticks, scada::StatusCode::Good, {}, {});
    return;
  }

  scada::base::Check(CheckInvariants());

  CoSpawn(
      executor_, weak_from_this(),
      [request_id, start_ticks, descriptions](
          std::shared_ptr<NodeFetcherImpl> self) mutable -> Awaitable<void> {
        auto result = co_await self->view_service_.Browse(
            self->service_context_, descriptions);
        auto status = result.status();
        auto results = std::move(result).value_or({});
        self->OnBrowseResult(request_id, start_ticks, std::move(status),
                             descriptions, std::move(results));
      });
}

void NodeFetcherImpl::NotifyFetchedNodes() {
  auto result = fetching_nodes_.GetFetchedNodes();
  if (result.nodes.empty() && result.errors.empty())
    return;

  LOG_INFO(logger_) << "Nodes fetched"
                    << LOG_TAG("NodeCount", result.nodes.size())
                    << LOG_TAG("ErrorCount", result.errors.size())
                    << LOG_TAG("FetchingCount", fetching_nodes_.size());
  LOG_DEBUG(logger_) << "Nodes fetched"
                     << LOG_TAG("NodeCount", result.nodes.size())
                     << LOG_TAG("ErrorCount", result.errors.size())
                     << LOG_TAG("FetchingCount", fetching_nodes_.size())
                     << LOG_TAG(
                            "NodeIds",
                            ToString(scada::base::AsList(
                                CollectFetchedNodeIds(result.nodes))))
                     << LOG_TAG("Errors",
                                ToString(scada::base::AsDict(result.errors)));

  // Consistency diagnostics over node data fetched from a (possibly remote)
  // server. The data is external, so inconsistencies are logged rather than
  // treated as internal invariants.
  {
    std::map<scada::NodeId, const scada::NodeState*> type_definition_map;
    for (auto& node : result.nodes) {
      if (scada::IsTypeDefinition(node.node_class))
        type_definition_map.emplace(node.node_id, &node);
    }
    for (auto& node : result.nodes) {
      if (scada::IsInstance(node.node_class)) {
        if (node.type_definition_id.is_null()) {
          LOG_WARNING(logger_)
              << "Fetched instance node has no type definition"
              << LOG_TAG("NodeId", NodeIdToScadaString(node.node_id));
          continue;
        }
        auto type_definition_id = node.type_definition_id;
        while (!type_definition_id.is_null()) {
          if (node_validator_(type_definition_id))
            break;
          auto i = type_definition_map.find(type_definition_id);
          if (i == type_definition_map.end()) {
            LOG_WARNING(logger_)
                << "Fetched node references an unknown type definition"
                << LOG_TAG("NodeId", NodeIdToScadaString(node.node_id))
                << LOG_TAG("TypeDefinitionId",
                           NodeIdToScadaString(type_definition_id));
            break;
          }
          auto* type_definiton = i->second;
          if (!scada::IsTypeDefinition(type_definiton->node_class)) {
            LOG_WARNING(logger_)
                << "Fetched type definition has a non-type node class"
                << LOG_TAG("NodeId",
                           NodeIdToScadaString(type_definiton->node_id));
            break;
          }
          type_definition_id = type_definiton->supertype_id;
        }
      }
    }
  }

  fetch_completed_handler_(std::move(result));
}

void NodeFetcherImpl::OnReadResult(
    unsigned request_id,
    scada::base::TimeTicks start_ticks,
    scada::Status&& status,
    const std::vector<scada::ReadValueId>& read_ids,
    std::vector<scada::DataValue>&& results) {
  scada::base::Check(CheckInvariants());

  // The response comes from a (possibly remote) service; a result-count
  // mismatch is handled below instead of being treated as an invariant.
  if (status && results.size() != read_ids.size()) {
    LOG_ERROR(logger_) << "Read request returned unexpected result count"
                       << LOG_TAG("RequestId", request_id)
                       << LOG_TAG("ExpectedCount", read_ids.size())
                       << LOG_TAG("ActualCount", results.size());
    status = scada::StatusCode::Bad;
    results.clear();
  }

  auto duration = scada::base::TimeTicks::Now() - start_ticks;
  LOG_INFO(logger_) << "Read request completed"
                    << LOG_TAG("RequestId", request_id)
                    << LOG_TAG("DurationMs", duration.InMilliseconds())
                    << LOG_TAG("Status", ToString(status));
  for (size_t i = 0; i < read_ids.size() && i < results.size(); ++i) {
    const auto& input = read_ids[i];
    LOG_DEBUG(logger_) << "Read request completed"
                       << LOG_TAG("RequestId", request_id)
                       << LOG_TAG("Input", ToString(input))
                       << LOG_TAG("StatusCode",
                                  status ? ToString(results[i].status_code)
                                         : ToString(status))
                       << LOG_TAG("Result", status ? ToString(results[i].value)
                                                   : std::string{});
  }

  {
    scada::base::Check(!processing_response_);
    scada::base::AutoReset processing_response{&processing_response_, true};

    LOG_INFO(logger_) << "Read request processing begin"
                      << LOG_TAG("RequestId", request_id)
                      << LOG_TAG("ReadCount", read_ids.size());

    auto result =
        status ? scada::DataValue{} : scada::MakeReadError(status.code());

    for (size_t i = 0; i < read_ids.size(); ++i) {
      if (status)
        ApplyReadResult(request_id, read_ids[i], std::move(results[i]));
      else
        ApplyReadResult(request_id, read_ids[i], scada::DataValue{result});
    }

    LOG_INFO(logger_) << "Read request processing completed"
                      << LOG_TAG("RequestId", request_id)
                      << LOG_TAG("ReadCount", read_ids.size());
  }

  scada::base::Check(running_request_count_ > 0);
  --running_request_count_;
  LOG_INFO(logger_) << "Remaining requests"
                    << LOG_TAG("RequestCount", running_request_count_);

  LOG_INFO(logger_) << "Read request notify fetched nodes begin"
                    << LOG_TAG("RequestId", request_id);
  NotifyFetchedNodes();
  LOG_INFO(logger_) << "Read request notify fetched nodes completed"
                    << LOG_TAG("RequestId", request_id);

  LOG_INFO(logger_) << "Read request fetch pending nodes begin"
                    << LOG_TAG("RequestId", request_id);
  FetchPendingNodes();
  LOG_INFO(logger_) << "Read request fetch pending nodes completed"
                    << LOG_TAG("RequestId", request_id);

  scada::base::Check(CheckInvariants());
}

void NodeFetcherImpl::ApplyReadResult(unsigned request_id,
                                      const scada::ReadValueId& read_id,
                                      scada::DataValue&& result) {
  auto* node = fetching_nodes_.FindNode(read_id.node_id);
  if (!node)
    return;

  // Request cancelation.
  if (IsEmpty(node->fetch_started) || node->fetch_request_id != request_id) {
    LOG_INFO(logger_) << "Ignore read response for canceled node"
                      << LOG_TAG("NodeId", ToString(node->node_state.node_id));
    return;
  }

  node->attributes_fetched = true;

  if (scada::IsBad(result.status_code)) {
    // Consider failure to obtain browse name as generic failure.
    if (read_id.attribute_id == scada::AttributeId::BrowseName)
      node->status = scada::Status{result.status_code};
    return;
  }

  SetFetchedAttribute(*node, read_id.attribute_id, std::move(result.value));
}

void NodeFetcherImpl::SetFetchedAttribute(FetchingNode& node,
                                          scada::AttributeId attribute_id,
                                          scada::Variant&& value) {
  switch (attribute_id) {
    case scada::AttributeId::NodeClass: {
      scada::Int32 node_class = 0;
      if (!value.get(node_class)) {
        LogUnexpectedAttributeValue(logger_, node, attribute_id, value);
        node.status = scada::StatusCode::Bad_WrongTypeId;
        return;
      }
      node.node_state.node_class = static_cast<scada::NodeClass>(node_class);
      break;
    }

    case scada::AttributeId::BrowseName: {
      scada::QualifiedName browse_name;
      if (!value.get(browse_name) || browse_name.empty()) {
        LogUnexpectedAttributeValue(logger_, node, attribute_id, value);
        node.status = scada::StatusCode::Bad_WrongTypeId;
        return;
      }
      node.node_state.attributes.browse_name = std::move(browse_name);
      break;
    }

    case scada::AttributeId::DisplayName: {
      scada::LocalizedText display_name;
      if (!value.get(display_name)) {
        LogUnexpectedAttributeValue(logger_, node, attribute_id, value);
        node.status = scada::StatusCode::Bad_WrongTypeId;
        return;
      }
      node.node_state.attributes.display_name = std::move(display_name);
      break;
    }

    case scada::AttributeId::DataType: {
      scada::NodeId data_type;
      if (!value.get(data_type) || data_type.is_null()) {
        LogUnexpectedAttributeValue(logger_, node, attribute_id, value);
        node.status = scada::StatusCode::Bad_WrongTypeId;
        return;
      }
      node.node_state.attributes.data_type = data_type;
      ValidateDependency(node, node.node_state.attributes.data_type);
      break;
    }

    case scada::AttributeId::Value:
      node.node_state.attributes.value = std::move(value);
      break;
  }
}

void NodeFetcherImpl::OnBrowseResult(
    unsigned request_id,
    scada::base::TimeTicks start_ticks,
    scada::Status&& status,
    const std::vector<scada::BrowseDescription>& descriptions,
    std::vector<scada::BrowseResult>&& results) {
  scada::base::Check(CheckInvariants());

  // The response comes from a (possibly remote) service; a result-count
  // mismatch is handled below instead of being treated as an invariant.
  if (status && results.size() != descriptions.size()) {
    LOG_ERROR(logger_) << "Browse request returned unexpected result count"
                       << LOG_TAG("RequestId", request_id)
                       << LOG_TAG("ExpectedCount", descriptions.size())
                       << LOG_TAG("ActualCount", results.size());
    status = scada::StatusCode::Bad;
    results.clear();
  }

  const auto duration = scada::base::TimeTicks::Now() - start_ticks;
  LOG_INFO(logger_) << "Browse request completed"
                    << LOG_TAG("RequestId", request_id)
                    << LOG_TAG("Count", descriptions.size())
                    << LOG_TAG("DurationMs", duration.InMilliseconds())
                    << LOG_TAG("Status", ToString(status));
  for (size_t i = 0; i < descriptions.size() && i < results.size(); ++i) {
    const auto& input = descriptions[i];
    LOG_DEBUG(logger_) << "Browse request completed"
                       << LOG_TAG("RequestId", request_id)
                       << LOG_TAG("Input", ToString(input))
                       << LOG_TAG("StatusCode",
                                  status ? ToString(results[i].status_code)
                                         : ToString(status))
                       << LOG_TAG("Result", status ? ToString(results[i])
                                                   : std::string{});
  }

  {
    scada::base::Check(!processing_response_);
    scada::base::AutoReset processing_response{&processing_response_, true};

    for (size_t i = 0; i < descriptions.size(); ++i) {
      if (status) {
        ApplyBrowseResult(request_id, descriptions[i], std::move(results[i]));
      } else {
        ApplyBrowseResult(request_id, descriptions[i],
                          scada::BrowseResult{status.code()});
      }
    }
  }

  scada::base::Check(running_request_count_ > 0);
  --running_request_count_;
  LOG_INFO(logger_) << "Remaining requests"
                    << LOG_TAG("RequestCount", running_request_count_);

  NotifyFetchedNodes();

  FetchPendingNodes();

  scada::base::Check(CheckInvariants());
}

void NodeFetcherImpl::ApplyBrowseResult(
    unsigned request_id,
    const scada::BrowseDescription& description,
    scada::BrowseResult&& result) {
  auto* node = fetching_nodes_.FindNode(description.node_id);
  if (!node)
    return;

  // Request cancelation.
  if (IsEmpty(node->fetch_started) || node->fetch_request_id != request_id) {
    LOG_INFO(logger_) << "Ignore browse response for canceled node"
                      << LOG_TAG("NodeId", ToString(node->node_state.node_id));
    return;
  }

  node->references_fetched = true;

  if (scada::IsBad(result.status_code)) {
    node->status = scada::Status{result.status_code};
    return;
  }

  for (auto& reference : result.references)
    AddFetchedReference(*node, description, std::move(reference));
}

void NodeFetcherImpl::AddFetchedReference(
    FetchingNode& node,
    const scada::BrowseDescription& description,
    scada::ReferenceDescription&& reference) {
  if (reference.reference_type_id.is_null() || reference.node_id.is_null()) {
    node.status = scada::Status{scada::StatusCode::Bad};
    return;
  }

  ValidateDependency(node, reference.reference_type_id);

  // Browse results come from a (possibly remote) server. Malformed or
  // conflicting references are external input: log and skip or overwrite
  // instead of panicking.
  if (description.direction == scada::BrowseDirection::Inverse) {
    // Parent.

    // The browse description is built by GetFetchReferences.
    scada::base::Check(description.reference_type_id ==
                           scada::id::HierarchicalReferences ||
                       description.reference_type_id == scada::id::HasSubtype);

    if (reference.forward) {
      LOG_WARNING(logger_) << "Ignore forward reference in an inverse browse"
                           << LOG_TAG("NodeId",
                                      ToString(node.node_state.node_id));
      return;
    }

    if (reference.reference_type_id == scada::id::HasSubtype) {
      if (!node.node_state.supertype_id.is_null() &&
          node.node_state.supertype_id != reference.node_id) {
        LOG_WARNING(logger_)
            << "Conflicting supertype in browse result"
            << LOG_TAG("NodeId", ToString(node.node_state.node_id));
      }
      node.node_state.supertype_id = reference.node_id;
    } else {
      if ((!node.node_state.parent_id.is_null() &&
           node.node_state.parent_id != reference.node_id) ||
          (!node.node_state.reference_type_id.is_null() &&
           node.node_state.reference_type_id != reference.reference_type_id)) {
        LOG_WARNING(logger_)
            << "Conflicting parent in browse result"
            << LOG_TAG("NodeId", ToString(node.node_state.node_id));
      }
      node.node_state.parent_id = reference.node_id;
      node.node_state.reference_type_id = reference.reference_type_id;
    }

    ValidateDependency(node, reference.node_id);

  } else if (description.reference_type_id == scada::id::Aggregates) {
    // Child.

    if (!reference.forward ||
        reference.reference_type_id == scada::id::HasSubtype) {
      LOG_WARNING(logger_) << "Ignore unexpected child reference"
                           << LOG_TAG("NodeId",
                                      ToString(node.node_state.node_id));
      return;
    }

    // Save parent for the pending child.
    // WARNING: |reference.node_id|.
    auto& child = fetching_nodes_.AddNode(reference.node_id);
    child.node_state.reference_type_id = reference.reference_type_id;
    child.node_state.parent_id = description.node_id;
    child.force |= node.force;

    if (reference.reference_type_id == scada::id::HasProperty) {
      child.node_state.node_class = scada::NodeClass::Variable;
      child.node_state.type_definition_id = scada::id::PropertyType;
      child.is_property = true;
      child.is_declaration =
          scada::IsTypeDefinition(node.node_state.node_class);

      // TODO: Optimize. May be done once.
      ValidateDependency(node, scada::id::PropertyType);
    }

    fetching_nodes_.AddDependency(child, node);
    fetching_nodes_.AddDependency(node, child);

    FetchNode(child, node.pending_sequence, NodeFetchStatus::NodeOnly, false);

  } else {
    // Non-hierarchical forward references, including HasTypeDefinition.

    // The browse description is built by GetFetchReferences.
    scada::base::Check(description.reference_type_id ==
                       scada::id::NonHierarchicalReferences);

    if (!reference.forward) {
      LOG_WARNING(logger_) << "Ignore inverse reference in a forward browse"
                           << LOG_TAG("NodeId",
                                      ToString(node.node_state.node_id));
      return;
    }

    if (reference.reference_type_id == scada::id::HasTypeDefinition) {
      if (!node.node_state.type_definition_id.is_null() &&
          node.node_state.type_definition_id != reference.node_id) {
        LOG_WARNING(logger_)
            << "Conflicting type definition in browse result"
            << LOG_TAG("NodeId", ToString(node.node_state.node_id));
      }
      node.node_state.type_definition_id = reference.node_id;
    } else if (std::find(node.node_state.references.begin(),
                         node.node_state.references.end(),
                         reference) == node.node_state.references.end()) {
      // Skip duplicate references returned by the server.
      node.node_state.references.emplace_back(reference);
    }

    ValidateDependency(node, reference.node_id);
  }
}

bool NodeFetcherImpl::CheckInvariants() const {
  return true;

  // Disabled full validation, kept as pure boolean logic for reference:
  /*for (auto& p : fetching_nodes_.fetching_nodes_) {
    auto& node = p.second;

    bool pending = !!pending_queue_.count(node);

    if (node.fetched()) {
      if (pending)
        return false;
      if (!node.fetch_started)
        return false;
    }
  }

  return fetching_nodes_.CheckInvariants();*/
}

void NodeFetcherImpl::ValidateDependency(FetchingNode& node,
                                         const scada::NodeId& from_id) {
  if (from_id.is_null())
    return;

  if (node.node_state.node_id == from_id)
    return;

  if (node_validator_(from_id))
    return;

  auto& from = fetching_nodes_.AddNode(from_id);

  // The equality is possible for references.
  if (&from != &node) {
    from.force |= node.force;
    fetching_nodes_.AddDependency(node, from);
    FetchNode(from, from.pending_sequence, NodeFetchStatus::NodeOnly, false);
  }
}
