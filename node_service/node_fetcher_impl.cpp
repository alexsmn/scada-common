#include "node_service/node_fetcher_impl.h"

#include "base/auto_reset.h"
#include "base/executor.h"
#include "base/range_util.h"
#include "core/attribute_service.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/view_service.h"

#include "base/debug_util-inl.h"

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/combine.hpp>

namespace {

const size_t kMaxRequestNodeCount = 100;
const size_t kMaxParallelRequestCount = 5;
// Read + Browse.
const size_t kPrimitiveRequestCount = 2;
const size_t kFetchAttributesReserveFactor = 5;
const size_t kFetchReferencesReserveFactor = 3;

void GetFetchAttributes(const FetchingNode& fetching_node,
                        std::vector<scada::ReadValueId>& read_ids) {
  const auto& node_id = fetching_node.node_state.node_id;

  read_ids.push_back({node_id, scada::AttributeId::BrowseName});

  // Have to fetch diplay names for instance properties as well, since property
  // declaration might not be created.
  read_ids.push_back({node_id, scada::AttributeId::DisplayName});

  if (!fetching_node.is_property || fetching_node.is_declaration)
    read_ids.push_back({node_id, scada::AttributeId::NodeClass});

  read_ids.push_back({node_id, scada::AttributeId::DataType});

  // Must read only property values.
  // Must read values of type definition properties to correctly read
  // EnumStrings.
  if (fetching_node.is_property)
    read_ids.push_back({node_id, scada::AttributeId::Value});
}

void GetFetchReferences(const FetchingNode& fetching_node,
                        std::vector<scada::BrowseDescription>& descriptions) {
  const auto& node_id = fetching_node.node_state.node_id;

  // Don't request references of a property.
  if (!fetching_node.is_property) {
    descriptions.push_back({node_id, scada::BrowseDirection::Forward,
                            scada::id::Aggregates, true});
  }

  // Request property categories.
  if (!fetching_node.is_property || fetching_node.is_declaration) {
    descriptions.push_back({node_id, scada::BrowseDirection::Forward,
                            scada::id::NonHierarchicalReferences, true});
  }

  if (fetching_node.fetch_started.non_hierarchical_inverse_references) {
    descriptions.push_back({node_id, scada::BrowseDirection::Inverse,
                            scada::id::NonHierarchicalReferences, true});
  }

  // Request parent if unknown. It may happen when node is created.
  const bool fetch_parent =
      node_id != scada::id::RootFolder &&
      fetching_node.node_state.parent_id.is_null() &&
      fetching_node.node_state.reference_type_id.is_null();
  if (fetch_parent) {
    descriptions.push_back({node_id, scada::BrowseDirection::Inverse,
                            scada::id::HierarchicalReferences, true});
  }
}

scada::NodeId FindSupertypeId(const scada::ReferenceDescriptions& references) {
  auto i = std::find_if(references.begin(), references.end(),
                        [](const scada::ReferenceDescription& ref) {
                          return ref.reference_type_id == scada::id::HasSubtype;
                        });
  return i != references.end() ? i->node_id : scada::NodeId{};
}

std::vector<scada::NodeId> CollectNodeIds(
    const std::vector<FetchingNode*> nodes) {
  return nodes | boost::adaptors::transformed([](const FetchingNode* node) {
           return node->node_state.node_id;
         }) |
         to_vector;
}

}  // namespace

NodeFetcherImpl::NodeFetcherImpl(NodeFetcherImplContext&& context)
    : NodeFetcherImplContext{std::move(context)} {}

NodeFetcherImpl::~NodeFetcherImpl() {}

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
  assert(!node_id.is_null());
  assert(status.node_fetched);
  assert(AssertValid());

  auto& node = fetching_nodes_.AddNode(node_id);

  FetchNode(node, next_pending_sequence_++, status, force);

  assert(AssertValid());
}

void NodeFetcherImpl::FetchNode(FetchingNode& node,
                                unsigned pending_sequence,
                                NodeFetchStatus status,
                                bool force) {
  assert(status.node_fetched);

  if (!node.fetch_started.empty()) {
    if (status.all_less_or_equal(node.fetch_started) && !force)
      return;

    LOG_INFO(logger_) << "Cancel started fetching node"
                      << LOG_TAG("NodeId", ToString(node.node_state.node_id))
                      << LOG_TAG("RequestId", node.fetch_request_id)
                      << LOG_TAG("FetchStarted", node.fetch_started);

    node.fetch_started = NodeFetchStatus{};
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
  assert(AssertValid());

  auto* fetching_node = fetching_nodes_.FindNode(node_id);
  if (!fetching_node)
    return;

  LOG_INFO(logger_) << "Cancel fetching node"
                    << LOG_TAG("NodeId", ToString(node_id));

  pending_queue_.erase(*fetching_node);

  fetching_nodes_.RemoveNode(node_id);

  NotifyFetchedNodes();

  FetchPendingNodes();

  assert(AssertValid());
}

void NodeFetcherImpl::FetchPendingNodes() {
  assert(AssertValid());

  if (processing_response_)
    return;

  if (pending_queue_.empty() ||
      running_request_count_ + kPrimitiveRequestCount >
          kMaxParallelRequestCount) {
    return;
  }

  std::vector<FetchingNode*> nodes;
  nodes.reserve(std::min(kMaxRequestNodeCount, pending_queue_.size()));

  while (!pending_queue_.empty() && nodes.size() < kMaxRequestNodeCount) {
    auto& node = pending_queue_.top();
    pending_queue_.pop();
    assert(node.fetch_started.empty());
    assert(!node.pending_status.empty());
    node.fetch_started = node.pending_status;
    nodes.emplace_back(&node);
  }

  FetchPendingNodes(std::move(nodes));

  assert(AssertValid());
}

unsigned NodeFetcherImpl::MakeRequestId() {
  const auto request_id = next_request_id_++;
  if (next_request_id_ == 0)
    next_request_id_ = 1;
  return request_id;
}

void NodeFetcherImpl::FetchPendingNodes(std::vector<FetchingNode*>&& nodes) {
  assert(AssertValid());

  const auto start_ticks = base::TimeTicks::Now();
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

  auto read_ids = std::make_shared<std::vector<scada::ReadValueId>>();
  read_ids->reserve(nodes.size() * kFetchAttributesReserveFactor);
  for (auto* node : nodes) {
    size_t count = read_ids->size();
    GetFetchAttributes(*node, *read_ids);
    node->attributes_fetched = count == read_ids->size();
  }

  assert(AssertValid());

  attribute_service_.Read(
      service_context_, read_ids,
      BindExecutor(
          executor_, weak_from_this(),
          [this, request_id, start_ticks, read_ids](
              scada::Status status, std::vector<scada::DataValue> results) {
            OnReadResult(request_id, start_ticks, std::move(status), *read_ids,
                         std::move(results));
          }));

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

  assert(AssertValid());

  view_service_.Browse(
      descriptions,
      BindExecutor(
          executor_, weak_from_this(),
          [this, request_id, start_ticks, descriptions](
              scada::Status status, std::vector<scada::BrowseResult> results) {
            OnBrowseResult(request_id, start_ticks, std::move(status),
                           std::move(descriptions), std::move(results));
          }));
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
                     << LOG_TAG("Nodes", ToString(result.nodes))
                     << LOG_TAG("Errors", ToString(result.errors));

#if !defined(NDEBUG)
  // Validation.
  {
    std::map<scada::NodeId, const scada::NodeState*> type_definition_map;
    for (auto& node : result.nodes) {
      if (scada::IsTypeDefinition(node.node_class))
        type_definition_map.emplace(node.node_id, &node);
    }
    for (auto& node : result.nodes) {
      if (scada::IsInstance(node.node_class)) {
        assert(!node.type_definition_id.is_null());
        auto type_definition_id = node.type_definition_id;
        while (!type_definition_id.is_null()) {
          if (node_validator_(type_definition_id))
            break;
          /*auto supertype_id = FindSupertypeId(node.references);
          assert(!supertype_id.is_null());*/
          assert(type_definition_map.count(type_definition_id));
          auto* type_definiton = type_definition_map[type_definition_id];
          assert(type_definiton);
          assert(scada::IsTypeDefinition(type_definiton->node_class));
          type_definition_id = type_definiton->supertype_id;
        }
      }
    }
  }
#endif

  fetch_completed_handler_(std::move(result));
}

void NodeFetcherImpl::OnReadResult(
    unsigned request_id,
    base::TimeTicks start_ticks,
    scada::Status&& status,
    const std::vector<scada::ReadValueId>& read_ids,
    std::vector<scada::DataValue>&& results) {
  assert(!status || results.size() == read_ids.size());
  assert(AssertValid());

  auto duration = base::TimeTicks::Now() - start_ticks;
  LOG_INFO(logger_) << "Read request completed"
                    << LOG_TAG("RequestId", request_id)
                    << LOG_TAG("DurationMs", duration.InMilliseconds())
                    << LOG_TAG("Status", ToString(status));
  for (size_t i = 0; i < read_ids.size(); ++i) {
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
    assert(!processing_response_);
    base::AutoReset processing_response{&processing_response_, true};

    auto result =
        status ? scada::DataValue{} : scada::MakeReadError(status.code());

    for (size_t i = 0; i < read_ids.size(); ++i) {
      if (status)
        ApplyReadResult(request_id, read_ids[i], std::move(results[i]));
      else
        ApplyReadResult(request_id, read_ids[i], scada::DataValue{result});
    }
  }

  assert(running_request_count_ > 0);
  --running_request_count_;
  LOG_INFO(logger_) << "Remaining requests"
                    << LOG_TAG("RequestCount", running_request_count_);

  NotifyFetchedNodes();

  FetchPendingNodes();

  assert(AssertValid());
}

void NodeFetcherImpl::ApplyReadResult(unsigned request_id,
                                      const scada::ReadValueId& read_id,
                                      scada::DataValue&& result) {
  auto* node = fetching_nodes_.FindNode(read_id.node_id);
  if (!node)
    return;

  // Request cancelation.
  if (node->fetch_started.empty() || node->fetch_request_id != request_id) {
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
    case scada::AttributeId::NodeClass:
      node.node_state.node_class =
          static_cast<scada::NodeClass>(value.as_int32());
      break;

    case scada::AttributeId::BrowseName:
      assert(!value.get<scada::QualifiedName>().empty());
      node.node_state.attributes.browse_name =
          std::move(value.get<scada::QualifiedName>());
      break;

    case scada::AttributeId::DisplayName:
      node.node_state.attributes.display_name =
          std::move(value.get<scada::LocalizedText>());
      break;

    case scada::AttributeId::DataType:
      assert(!value.as_node_id().is_null());
      node.node_state.attributes.data_type =
          std::move(value.get<scada::NodeId>());
      ValidateDependency(node, node.node_state.attributes.data_type);
      break;

    case scada::AttributeId::Value:
      node.node_state.attributes.value = std::move(value);
      break;
  }
}

void NodeFetcherImpl::OnBrowseResult(
    unsigned request_id,
    base::TimeTicks start_ticks,
    scada::Status&& status,
    const std::vector<scada::BrowseDescription>& descriptions,
    std::vector<scada::BrowseResult>&& results) {
  assert(!status || results.size() == descriptions.size());
  assert(AssertValid());

  const auto duration = base::TimeTicks::Now() - start_ticks;
  LOG_INFO(logger_) << "Browse request completed"
                    << LOG_TAG("RequestId", request_id)
                    << LOG_TAG("Count", descriptions.size())
                    << LOG_TAG("DurationMs", duration.InMilliseconds())
                    << LOG_TAG("Status", ToString(status));
  for (size_t i = 0; i < descriptions.size(); ++i) {
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
    assert(!processing_response_);
    base::AutoReset processing_response{&processing_response_, true};

    for (size_t i = 0; i < descriptions.size(); ++i) {
      if (status) {
        ApplyBrowseResult(request_id, descriptions[i], std::move(results[i]));
      } else {
        ApplyBrowseResult(request_id, descriptions[i],
                          scada::BrowseResult{status.code()});
      }
    }
  }

  assert(running_request_count_ > 0);
  --running_request_count_;
  LOG_INFO(logger_) << "Remaining requests"
                    << LOG_TAG("RequestCount", running_request_count_);

  NotifyFetchedNodes();

  FetchPendingNodes();

  assert(AssertValid());
}

void NodeFetcherImpl::ApplyBrowseResult(
    unsigned request_id,
    const scada::BrowseDescription& description,
    scada::BrowseResult&& result) {
  auto* node = fetching_nodes_.FindNode(description.node_id);
  if (!node)
    return;

  // Request cancelation.
  if (node->fetch_started.empty() || node->fetch_request_id != request_id) {
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
  if (reference.reference_type_id.is_null())
    throw std::exception{};
  if (reference.node_id.is_null())
    throw std::exception{};

  ValidateDependency(node, reference.reference_type_id);

  if (description.direction == scada::BrowseDirection::Inverse) {
    // Parent.

    assert(!reference.forward);
    assert(description.reference_type_id == scada::id::HierarchicalReferences ||
           description.reference_type_id == scada::id::HasSubtype);

    if (reference.reference_type_id == scada::id::HasSubtype) {
      assert(node.node_state.supertype_id.is_null() ||
             node.node_state.supertype_id == reference.node_id);
      node.node_state.supertype_id = reference.node_id;
    } else {
      assert(node.node_state.parent_id.is_null() ||
             node.node_state.parent_id == reference.node_id);
      assert(node.node_state.reference_type_id.is_null() ||
             node.node_state.reference_type_id == reference.reference_type_id);
      assert(reference.reference_type_id != scada::id::HasSubtype);
      node.node_state.parent_id = reference.node_id;
      node.node_state.reference_type_id = reference.reference_type_id;
    }

    ValidateDependency(node, reference.node_id);

  } else if (description.reference_type_id == scada::id::Aggregates) {
    // Child.

    assert(reference.forward);
    assert(reference.reference_type_id != scada::id::HasSubtype);

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

    FetchNode(child, node.pending_sequence, NodeFetchStatus::NodeOnly(), false);

  } else {
    // Non-hierarchical forward references, including HasTypeDefinition.

    assert(reference.forward);
    assert(description.reference_type_id ==
           scada::id::NonHierarchicalReferences);

    if (reference.reference_type_id == scada::id::HasTypeDefinition) {
      assert(node.node_state.type_definition_id.is_null() ||
             node.node_state.type_definition_id == reference.node_id);
      node.node_state.type_definition_id = reference.node_id;
    } else {
      assert(std::find(node.node_state.references.begin(),
                       node.node_state.references.end(),
                       reference) == node.node_state.references.end());
      node.node_state.references.emplace_back(reference);
    }

    ValidateDependency(node, reference.node_id);
  }
}

bool NodeFetcherImpl::AssertValid() const {
  return true;

  /*for (auto& p : fetching_nodes_.fetching_nodes_) {
    auto& node = p.second;

    bool pending = !!pending_queue_.count(node);

    if (node.fetched()) {
      assert(!pending);

      assert(node.fetch_started);
      if (!node.fetch_started)
        return false;

    } else {
      //assert(pending || node.fetch_started);
      //if (!pending && !node.fetch_started)
      //  return false;*
    }

    if (pending) {
      assert(!node.fetched());
      if (node.fetched())
        return false;
    }
  }

  return fetching_nodes_.AssertValid();*/
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
    FetchNode(from, from.pending_sequence, NodeFetchStatus::NodeOnly(), false);
  }
}
