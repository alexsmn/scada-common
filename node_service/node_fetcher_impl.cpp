#include "node_service/node_fetcher_impl.h"

#include "base/executor.h"
#include "core/attribute_service.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/view_service.h"

#include "core/debug_util-inl.h"

namespace {

const size_t kMaxFetchNodeCount = 1000;

const size_t kFetchAttributesReserveFactor = 5;

void GetFetchAttributes(const scada::NodeId& node_id,
                        bool is_property,
                        bool is_declaration,
                        std::vector<scada::ReadValueId>& read_ids) {
  read_ids.push_back({node_id, scada::AttributeId::BrowseName});

  // Have to fetch diplay names for instance properties as well, since property
  // declaration might not be created.
  read_ids.push_back({node_id, scada::AttributeId::DisplayName});

  if (!is_property || is_declaration)
    read_ids.push_back({node_id, scada::AttributeId::NodeClass});

  read_ids.push_back({node_id, scada::AttributeId::DataType});

  // Must read only property values.
  // Must read values of type definition properties to correctly read
  // EnumStrings.
  if (is_property)
    read_ids.push_back({node_id, scada::AttributeId::Value});
}

const size_t kFetchReferencesReserveFactor = 3;

void GetFetchReferences(const scada::NodeState& node,
                        bool is_property,
                        bool is_declaration,
                        bool fetch_parent,
                        std::vector<scada::BrowseDescription>& descriptions) {
  // Don't request references of a property.
  if (!is_property) {
    descriptions.push_back({node.node_id, scada::BrowseDirection::Forward,
                            scada::id::Aggregates, true});
  }

  // Request property categories.
  if (!is_property || is_declaration) {
    descriptions.push_back({node.node_id, scada::BrowseDirection::Forward,
                            scada::id::NonHierarchicalReferences, true});
  }

  // Request parent if unknown. It may happen when node is created.
  if (node.node_id != scada::id::RootFolder) {
    if (fetch_parent) {
      descriptions.push_back({node.node_id, scada::BrowseDirection::Inverse,
                              scada::id::HierarchicalReferences, true});
    } else {
      descriptions.push_back({node.node_id, scada::BrowseDirection::Inverse,
                              scada::id::HasSubtype, true});
    }
  }
}

template <class T>
inline void Insert(std::vector<T>& c, const T& v) {
  if (std::find(c.begin(), c.end(), v) == c.end())
    c.push_back(v);
}

template <class T>
inline void Erase(std::vector<T>& c, const T& v) {
  auto i = std::find(c.begin(), c.end(), v);
  if (i != c.end())
    c.erase(i);
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

NodeFetcherImpl::FetchingNode* NodeFetcherImpl::FetchingNodeGraph::FindNode(
    const scada::NodeId& node_id) {
  assert(!node_id.is_null());

  auto i = fetching_nodes_.find(node_id);
  return i != fetching_nodes_.end() ? &i->second : nullptr;
}

NodeFetcherImpl::FetchingNode& NodeFetcherImpl::FetchingNodeGraph::AddNode(
    const scada::NodeId& node_id) {
  assert(!node_id.is_null());

  auto p = fetching_nodes_.try_emplace(node_id);
  auto& node = p.first->second;
  if (p.second)
    node.node_id = node_id;

  return node;
}

void NodeFetcherImpl::Fetch(const scada::NodeId& node_id,
                            bool fetch_parent,
                            const std::optional<ParentInfo> parent_info,
                            bool force) {
  assert(AssertValid());
  assert(!parent_info.has_value() ||
         parent_info->reference_type_id != scada::id::HasSubtype);

  auto& node = fetching_nodes_.AddNode(node_id);
  node.fetch_parent |= fetch_parent;
  node.force |= force;

  if (parent_info) {
    assert(node.parent_id.is_null() ||
           node.parent_id == parent_info->parent_id);
    assert(node.reference_type_id.is_null() ||
           node.reference_type_id == parent_info->reference_type_id);
    node.parent_id = parent_info->parent_id;
    node.reference_type_id = parent_info->reference_type_id;

    ValidateDependency(node, node.parent_id);
    ValidateDependency(node, node.reference_type_id);
  }

  FetchNode(node, next_pending_sequence_++);

  assert(AssertValid());
}

void NodeFetcherImpl::FetchNode(FetchingNode& node, unsigned pending_sequence) {
  if (pending_queue_.count(node))
    return;

  if (node.fetch_started) {
    if (!node.force)
      return;

    LOG_INFO(logger_) << "Cancel started fetching node"
                      << LOG_TAG("NodeId", ToString(node.node_id))
                      << LOG_TAG("RequestId", node.fetch_request_id);
    node.fetch_started = false;
    node.fetch_request_id = 0;
    node.attributes_fetched = false;
    node.references_fetched = false;
    node.status = scada::StatusCode::Good;
  }

  // LOG_INFO(logger_) << "Schedule fetch node"
  //                   << LOG_TAG("NodeId", ToString(node.node_id));

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

void NodeFetcherImpl::FetchingNodeGraph::RemoveNode(
    const scada::NodeId& node_id) {
  assert(AssertValid());

  auto i = fetching_nodes_.find(node_id);
  if (i == fetching_nodes_.end())
    return;

  auto& node = i->second;
  node.ClearDependsOf();
  node.ClearDependentNodes();

  fetching_nodes_.erase(i);

  assert(AssertValid());
}

void NodeFetcherImpl::FetchPendingNodes() {
  assert(AssertValid());

  if (pending_queue_.empty() || running_request_count_ != 0)
    return;

  std::vector<FetchingNode*> nodes;
  nodes.reserve(std::min(kMaxFetchNodeCount, pending_queue_.size()));

  while (!pending_queue_.empty() && nodes.size() < kMaxFetchNodeCount) {
    auto& node = pending_queue_.top();
    pending_queue_.pop();
    assert(!node.fetch_started);
    node.fetch_started = true;
    nodes.emplace_back(&node);
  }

  FetchPendingNodes(std::move(nodes));

  assert(AssertValid());
}

void NodeFetcherImpl::FetchPendingNodes(std::vector<FetchingNode*>&& nodes) {
  assert(AssertValid());

  const auto start_ticks = base::TimeTicks::Now();

  const auto request_id = next_request_id_++;
  if (next_request_id_ == 0)
    next_request_id_ = 1;

  LOG_INFO(logger_) << "Fetch pending nodes"
                    << LOG_TAG("NodeIds", ToString(CollectNodeIds(nodes)))
                    << LOG_TAG("RequestId", request_id)
                    << LOG_TAG("RequestCount", running_request_count_);

  // Increment immediately for the case OnReadResult happens synchronously.
  running_request_count_ += 2;

  // Attributes

  auto read_ids = std::make_shared<std::vector<scada::ReadValueId>>();
  read_ids->reserve(nodes.size() * kFetchAttributesReserveFactor);
  for (auto* node : nodes) {
    size_t count = read_ids->size();
    GetFetchAttributes(node->node_id, node->is_property, node->is_declaration,
                       *read_ids);
    node->attributes_fetched = count == read_ids->size();
    node->fetch_request_id = request_id;
  }

  assert(AssertValid());

  attribute_service_.Read(
      *read_ids,
      BindExecutor(
          executor_,
          [weak_ptr = weak_from_this(), request_id, start_ticks, read_ids](
              scada::Status status, std::vector<scada::DataValue> results) {
            if (auto ptr = weak_ptr.lock()) {
              ptr->OnReadResult(request_id, start_ticks, std::move(status),
                                *read_ids, std::move(results));
            }
          }));

  // References

  std::vector<scada::BrowseDescription> descriptions;
  descriptions.reserve(nodes.size() * kFetchReferencesReserveFactor);
  for (auto* node : nodes) {
    size_t count = descriptions.size();
    GetFetchReferences(*node, node->is_property, node->is_declaration,
                       node->fetch_parent, descriptions);
    node->references_fetched = count == descriptions.size();
  }

  if (descriptions.empty()) {
    OnBrowseResult(request_id, start_ticks, scada::StatusCode::Good, {}, {});
    return;
  }

  assert(AssertValid());

  view_service_.Browse(
      descriptions,
      [weak_ptr = weak_from_this(), request_id, start_ticks, descriptions](
          scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        if (auto ptr = weak_ptr.lock()) {
          ptr->OnBrowseResult(request_id, start_ticks, std::move(status),
                              std::move(descriptions), std::move(results));
        }
      });
}

void NodeFetcherImpl::NotifyFetchedNodes() {
  auto [nodes, errors] = fetching_nodes_.GetFetchedNodes();
  if (nodes.empty() && errors.empty())
    return;

  LOG_INFO(logger_) << "Nodes fetched" << LOG_TAG("NodeCount", nodes.size())
                    << LOG_TAG("ErrorCount", errors.size())
                    << LOG_TAG("FetchingCount", fetching_nodes_.size())
                    << LOG_TAG("Nodes", ToString(nodes))
                    << LOG_TAG("Errors", ToString(errors));

  fetch_completed_handler_(std::move(nodes), std::move(errors));
}

void NodeFetcherImpl::FetchingNode::ClearDependentNodes() {
  for (auto* dependent_node : dependent_nodes)
    Erase(dependent_node->depends_of, this);
  dependent_nodes.clear();
}

void NodeFetcherImpl::FetchingNode::ClearDependsOf() {
  for (auto* depends_of : depends_of)
    Erase(depends_of->dependent_nodes, this);
  depends_of.clear();
}

std::pair<std::vector<scada::NodeState> /*fetched_nodes*/,
          NodeFetchStatuses /*errors*/>
NodeFetcherImpl::FetchingNodeGraph::GetFetchedNodes() {
  assert(AssertValid());

  struct Collector {
    bool IsFetchedRecursively(FetchingNode& node) {
      if (!node.fetched())
        return false;

      if (node.fetch_cache_iteration == fetch_cache_iteration)
        return node.fetch_cache_state;

      // A cycle means fetched as well.
      node.fetch_cache_iteration = fetch_cache_iteration;
      node.fetch_cache_state = true;

      bool fetched = true;
      for (auto* depends_of : node.depends_of) {
        if (!IsFetchedRecursively(*depends_of)) {
          fetched = false;
          break;
        }
      }

      node.fetch_cache_state = fetched;

      return fetched;
    }

    FetchingNodeGraph& graph;
    int fetch_cache_iteration = 0;
  };

  std::vector<scada::NodeState> fetched_nodes;
  fetched_nodes.reserve(std::min<size_t>(32, fetching_nodes_.size()));

  NodeFetchStatuses errors;

  Collector collector{*this};
  for (auto i = fetching_nodes_.begin(); i != fetching_nodes_.end();) {
    auto& node = i->second;

    collector.fetch_cache_iteration = fetch_cache_iteration_++;
    if (!collector.IsFetchedRecursively(node)) {
      ++i;
      continue;
    }

    assert(node.fetch_started);

    if (node.status)
      fetched_nodes.emplace_back(node);
    else
      errors.emplace_back(node.node_id, node.status);

    node.ClearDependsOf();
    node.ClearDependentNodes();
    i = fetching_nodes_.erase(i);
  }

  assert(AssertValid());

  return {std::move(fetched_nodes), std::move(errors)};
}

void NodeFetcherImpl::OnReadResult(
    unsigned request_id,
    base::TimeTicks start_ticks,
    scada::Status&& status,
    const std::vector<scada::ReadValueId>& read_ids,
    std::vector<scada::DataValue>&& results) {
  assert(AssertValid());

  auto duration = base::TimeTicks::Now() - start_ticks;
  LOG_INFO(logger_) << "Read request completed"
                    << LOG_TAG("RequestId", request_id)
                    << LOG_TAG("DurationMs", duration.InMilliseconds())
                    << LOG_TAG("Status", ToString(status))
                    << LOG_TAG("Inputs", ToString(read_ids))
                    << LOG_TAG("Results", ToString(results));

  for (size_t i = 0; i < read_ids.size(); ++i) {
    auto& read_id = read_ids[i];

    auto* node = fetching_nodes_.FindNode(read_id.node_id);
    if (!node)
      continue;

    // Request cancelation.
    if (!node->fetch_started || node->fetch_request_id != request_id) {
      LOG_INFO(logger_) << "Ignore read response for canceled node"
                        << LOG_TAG("NodeId", ToString(node->node_id));
      continue;
    }

    node->attributes_fetched = true;

    if (!status) {
      node->status = status;
      continue;
    }

    assert(read_ids.size() == results.size());
    auto& result = results[i];

    if (!scada::Status{result.status_code}) {
      // Consider failure to obtain browse name as generic failure.
      if (read_id.attribute_id == scada::AttributeId::BrowseName)
        node->status = scada::Status{result.status_code};
      continue;
    }

    SetFetchedAttribute(*node, read_id.attribute_id, std::move(result.value));
  }

  assert(running_request_count_ > 0);
  --running_request_count_;
  LOG_INFO(logger_) << "Remaining requests"
                    << LOG_TAG("RequestCount", running_request_count_);

  NotifyFetchedNodes();

  FetchPendingNodes();

  assert(AssertValid());
}

void NodeFetcherImpl::SetFetchedAttribute(FetchingNode& node,
                                          scada::AttributeId attribute_id,
                                          scada::Variant&& value) {
  switch (attribute_id) {
    case scada::AttributeId::NodeClass:
      node.node_class = static_cast<scada::NodeClass>(value.as_int32());
      break;

    case scada::AttributeId::BrowseName:
      assert(!value.get<scada::QualifiedName>().empty());
      node.attributes.browse_name =
          std::move(value.get<scada::QualifiedName>());
      break;

    case scada::AttributeId::DisplayName:
      node.attributes.display_name = std::move(value.as_localized_text());
      break;

    case scada::AttributeId::DataType:
      assert(!value.as_node_id().is_null());
      node.attributes.data_type = std::move(value.as_node_id());
      ValidateDependency(node, node.attributes.data_type);
      break;

    case scada::AttributeId::Value:
      node.attributes.value = std::move(value);
      break;
  }
}

void NodeFetcherImpl::OnBrowseResult(
    unsigned request_id,
    base::TimeTicks start_ticks,
    scada::Status&& status,
    const std::vector<scada::BrowseDescription>& descriptions,
    std::vector<scada::BrowseResult>&& results) {
  assert(AssertValid());

  const auto duration = base::TimeTicks::Now() - start_ticks;
  LOG_INFO(logger_) << "Browse request completed"
                    << LOG_TAG("RequestId", request_id)
                    << LOG_TAG("Count", descriptions.size())
                    << LOG_TAG("DurationMs", duration.InMilliseconds())
                    << LOG_TAG("Status", ToString(status))
                    << LOG_TAG("Inputs", ToString(descriptions))
                    << LOG_TAG("Results", ToString(results));

  for (size_t i = 0; i < descriptions.size(); ++i) {
    auto& description = descriptions[i];

    auto* node = fetching_nodes_.FindNode(description.node_id);
    if (!node)
      continue;

    // Request cancelation.
    if (!node->fetch_started || node->fetch_request_id != request_id) {
      LOG_INFO(logger_) << "Ignore browse response for canceled node"
                        << LOG_TAG("NodeId", ToString(node->node_id));
      continue;
    }

    node->references_fetched = true;

    if (!status) {
      node->status = status;
      continue;
    }

    assert(descriptions.size() == results.size());
    auto& result = results[i];

    if (!scada::Status{result.status_code}) {
      node->status = scada::Status{result.status_code};
      continue;
    }

    for (auto& reference : result.references)
      AddFetchedReference(*node, description, std::move(reference));
  }

  assert(running_request_count_ > 0);
  --running_request_count_;
  LOG_INFO(logger_) << "Remaining requests"
                    << LOG_TAG("RequestCount", running_request_count_);

  NotifyFetchedNodes();

  FetchPendingNodes();

  assert(AssertValid());
}

void NodeFetcherImpl::FetchingNodeGraph::AddDependency(FetchingNode& node,
                                                       FetchingNode& from) {
  assert(&node != &from);

  Insert(from.dependent_nodes, &node);
  Insert(node.depends_of, &from);
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
      assert(node.supertype_id.is_null() ||
             node.supertype_id == reference.node_id);
      node.supertype_id = reference.node_id;
    } else {
      assert(node.parent_id.is_null() || node.parent_id == reference.node_id);
      assert(node.reference_type_id.is_null() ||
             node.reference_type_id == reference.reference_type_id);
      assert(reference.reference_type_id != scada::id::HasSubtype);
      node.parent_id = reference.node_id;
      node.reference_type_id = reference.reference_type_id;
    }

    ValidateDependency(node, reference.node_id);

  } else if (description.reference_type_id == scada::id::Aggregates) {
    // Child.

    assert(reference.forward);
    assert(reference.reference_type_id != scada::id::HasSubtype);

    // Save parent for the pending child.
    // WARNING: |reference.node_id|.
    auto& child = fetching_nodes_.AddNode(reference.node_id);
    child.reference_type_id = reference.reference_type_id;
    child.parent_id = description.node_id;
    child.force |= node.force;

    if (reference.reference_type_id == scada::id::HasProperty) {
      child.node_class = scada::NodeClass::Variable;
      child.type_definition_id = scada::id::PropertyType;
      child.is_property = true;
      child.is_declaration = scada::IsTypeDefinition(node.node_class);

      // TODO: Optimize. May be done once.
      ValidateDependency(node, scada::id::PropertyType);
    }

    fetching_nodes_.AddDependency(child, node);
    fetching_nodes_.AddDependency(node, child);

    FetchNode(child, node.pending_sequence);

  } else {
    // Non-hierarchical forward references, including HasTypeDefinition.

    assert(reference.forward);
    assert(description.reference_type_id ==
           scada::id::NonHierarchicalReferences);

    if (reference.reference_type_id == scada::id::HasTypeDefinition) {
      assert(node.type_definition_id.is_null() ||
             node.type_definition_id == reference.node_id);
      node.type_definition_id = reference.node_id;
    } else {
      assert(std::find(node.references.begin(), node.references.end(),
                       reference) == node.references.end());
      node.references.emplace_back(reference);
    }

    ValidateDependency(node, reference.node_id);
  }
}

bool NodeFetcherImpl::AssertValid() const {
  return true;

  for (auto& p : fetching_nodes_.fetching_nodes_) {
    auto& node = p.second;

    bool pending = !!pending_queue_.count(node);

    if (node.fetched()) {
      assert(!pending);

      assert(node.fetch_started);
      if (!node.fetch_started)
        return false;

    } else {
      /*assert(pending || node.fetch_started);
      if (!pending && !node.fetch_started)
        return false;*/
    }

    if (pending) {
      assert(!node.fetched());
      if (node.fetched())
        return false;
    }
  }

  return fetching_nodes_.AssertValid();
}

bool NodeFetcherImpl::FetchingNodeGraph::AssertValid() const {
  return true;
}

std::string NodeFetcherImpl::FetchingNodeGraph::GetDebugString() const {
  std::stringstream stream;
  for (auto& p : fetching_nodes_) {
    auto& node = p.second;
    stream << node.node_id << ": ";
    for (auto* depends_of : node.depends_of)
      stream << depends_of->node_id << ", ";
    stream << std::endl;
  }
  return stream.str();
}

void NodeFetcherImpl::ValidateDependency(FetchingNode& node,
                                         const scada::NodeId& from_id) {
  if (from_id.is_null())
    return;

  if (node.node_id == from_id)
    return;

  if (node_validator_(from_id))
    return;

  auto& from = fetching_nodes_.AddNode(from_id);

  // The equality is possible for references.
  if (&from != &node) {
    from.fetch_parent = true;
    from.force |= node.force;
    fetching_nodes_.AddDependency(node, from);
    FetchNode(from, from.pending_sequence);
  }
}

// static
std::vector<scada::NodeId> NodeFetcherImpl::CollectNodeIds(
    const std::vector<FetchingNode*> nodes) {
  std::vector<scada::NodeId> result;
  result.reserve(nodes.size());
  for (auto* node : nodes)
    result.push_back(node->node_id);
  return result;
}

// NodeFetcherImpl::PendingQueue

void NodeFetcherImpl::PendingQueue::push(FetchingNode& node) {
  if (node.pending) {
    assert(find(node) != queue_.end());
    return;
  }

  assert(find(node) == queue_.end());

  node.pending = true;

  queue_.emplace_back(Node{next_sequence_++, &node});
  std::push_heap(queue_.begin(), queue_.end());
}

void NodeFetcherImpl::PendingQueue::pop() {
  auto& node = *queue_.front().node;
  assert(node.pending);
  node.pending = false;
  std::pop_heap(queue_.begin(), queue_.end());
  queue_.pop_back();
}

void NodeFetcherImpl::PendingQueue::erase(FetchingNode& node) {
  if (!node.pending) {
    assert(find(node) == queue_.end());
    return;
  }

  auto i = find(node);
  assert(i != queue_.end());

  if (i != std::prev(queue_.end()))
    std::iter_swap(i, std::prev(queue_.end()));
  queue_.erase(std::prev(queue_.end()));

  std::make_heap(queue_.begin(), queue_.end());

  node.pending = false;
}

std::vector<NodeFetcherImpl::PendingQueue::Node>::iterator
NodeFetcherImpl::PendingQueue::find(const FetchingNode& node) {
  return std::find_if(queue_.begin(), queue_.end(),
                      [&node](const Node& n) { return n.node == &node; });
}

std::vector<NodeFetcherImpl::PendingQueue::Node>::const_iterator
NodeFetcherImpl::PendingQueue::find(const FetchingNode& node) const {
  return std::find_if(queue_.cbegin(), queue_.cend(),
                      [&node](const Node& n) { return n.node == &node; });
}

std::size_t NodeFetcherImpl::PendingQueue::count(
    const FetchingNode& node) const {
  assert(node.pending == (find(node) != queue_.cend()));
  return node.pending;
}
