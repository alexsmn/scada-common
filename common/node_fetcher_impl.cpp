#include "common/node_fetcher_impl.h"

#include "base/logger.h"
#include "common/node_id_util.h"
#include "core/attribute_service.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/view_service.h"

namespace {

const size_t kMaxFetchNodeCount = 20;

const scada::AttributeId kAttributeIds[] = {
    scada::AttributeId::NodeClass, scada::AttributeId::BrowseName,
    scada::AttributeId::DisplayName,
    // scada::AttributeId::Description,
    // scada::AttributeId::WriteMask,
    // scada::AttributeId::UserWriteMask,
    // scada::AttributeId::IsAbstract,
    // scada::AttributeId::Symmetric,
    // scada::AttributeId::InverseName,
    // scada::AttributeId::ContainsNoLoops,
    // scada::AttributeId::EventNotifier,
    scada::AttributeId::Value, scada::AttributeId::DataType,
    // scada::AttributeId::ValueRank,
    // scada::AttributeId::ArrayDimensions,
    // scada::AttributeId::AccessLevel,
    // scada::AttributeId::UserAccessLevel,
    // scada::AttributeId::MinimumSamplingInterval,
    // scada::AttributeId::Historizing,
    // scada::AttributeId::Executable,
    // scada::AttributeId::UserExecutable,
};

const size_t kFetchAttributesReserveFactor = std::size(kAttributeIds);

void GetFetchAttributes(const scada::NodeId& node_id,
                        bool is_property,
                        bool is_declaration,
                        std::vector<scada::ReadValueId>& read_ids) {
  read_ids.push_back({node_id, scada::AttributeId::BrowseName});

  if (!is_property || is_declaration) {
    read_ids.push_back({node_id, scada::AttributeId::NodeClass});
    read_ids.push_back({node_id, scada::AttributeId::DisplayName});
  }

  read_ids.push_back({node_id, scada::AttributeId::DataType});

  // Must read only property values.
  // Must read values of type definition properties to correctly read
  // EnumStrings.
  if (is_property)
    read_ids.push_back({node_id, scada::AttributeId::Value});
}

const size_t kFetchReferencesReserveFactor = 3;

void GetFetchReferences(const scada::NodeState& node,
                        bool fetch_parent,
                        std::vector<scada::BrowseDescription>& descriptions) {
  // Don't request references of a property.
  if (node.reference_type_id != scada::id::HasProperty) {
    descriptions.push_back({node.node_id, scada::BrowseDirection::Forward,
                            scada::id::Aggregates, true});
    descriptions.push_back({node.node_id, scada::BrowseDirection::Forward,
                            scada::id::NonHierarchicalReferences, true});
  }

  // Request parent if unknown. It may happen when node is created.
  if (fetch_parent && node.node_id != scada::id::RootFolder) {
    descriptions.push_back({node.node_id, scada::BrowseDirection::Inverse,
                            scada::id::HierarchicalReferences, true});
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
                            const scada::NodeId& parent_id,
                            const scada::NodeId& reference_type_id,
                            bool force) {
  assert(AssertValid());

  auto& node = fetching_nodes_.AddNode(node_id);
  node.fetch_parent |= fetch_parent;
  node.force |= force;

  if (!parent_id.is_null()) {
    assert(!reference_type_id.is_null());
    assert(node.parent_id.is_null() || node.parent_id == parent_id);
    assert(node.reference_type_id.is_null() ||
           node.reference_type_id == reference_type_id);
    node.parent_id = parent_id;
    node.reference_type_id = reference_type_id;

    ValidateDependency(node, parent_id);
    ValidateDependency(node, reference_type_id);
  }

  FetchNode(node);

  assert(AssertValid());
}

void NodeFetcherImpl::FetchNode(FetchingNode& node) {
  if (node.pending)
    return;

  if (node.fetch_started) {
    if (!node.force)
      return;

    logger_->WriteF(LogSeverity::Normal,
                    "Canceling old fetching node %s with request %u",
                    node.node_id.ToString().c_str(), node.fetch_request_id);
    node.fetch_started = false;
    node.fetch_request_id = 0;
    node.attributes_fetched = false;
    node.references_fetched = false;
    node.status = scada::StatusCode::Good;
  }

  logger_->WriteF(LogSeverity::Normal, "Scheduling fetch node %s",
                  node.node_id.ToString().c_str());

  assert(!node.pending);
  node.pending = true;
  assert(std::find(pending_queue_.begin(), pending_queue_.end(), &node) ==
         pending_queue_.end());
  pending_queue_.emplace_back(&node);

  FetchPendingNodes();
}

void NodeFetcherImpl::Cancel(const scada::NodeId& node_id) {
  assert(AssertValid());

  logger_->WriteF(LogSeverity::Normal, "Cancel fetch node %s",
                  node_id.ToString().c_str());

  auto* fetching_node = fetching_nodes_.FindNode(node_id);
  if (!fetching_node)
    return;

  if (fetching_node->pending) {
    auto i =
        std::find(pending_queue_.begin(), pending_queue_.end(), fetching_node);
    assert(i != pending_queue_.end());
    pending_queue_.erase(i);
    fetching_node->pending = false;
  }

  assert(std::find(pending_queue_.begin(), pending_queue_.end(),
                   fetching_node) == pending_queue_.end());

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
  if (pending_queue_.empty() || running_request_count_ != 0)
    return;

  std::vector<FetchingNode*> nodes;
  nodes.reserve(std::min(kMaxFetchNodeCount, pending_queue_.size()));

  while (!pending_queue_.empty() && nodes.size() < kMaxFetchNodeCount) {
    auto& node = *pending_queue_.front();
    assert(node.pending);
    assert(!node.fetch_started);
    pending_queue_.pop_front();
    node.pending = false;
    node.fetch_started = true;
    nodes.emplace_back(&node);
  }

  FetchPendingNodes(std::move(nodes));

  assert(AssertValid());
}

void NodeFetcherImpl::FetchPendingNodes(std::vector<FetchingNode*>&& nodes) {
  logger_->WriteF(LogSeverity::Normal, "Fetch nodes (%Iu)", nodes.size());

  // Increment immediately for the case OnReadResult happens synchronously.
  running_request_count_ += 2;
  logger_->WriteF(LogSeverity::Normal, "%Iu requests are running",
                  running_request_count_);

  const auto start_ticks = base::TimeTicks::Now();

  const auto request_id = next_request_id_++;
  if (next_request_id_ == 0)
    next_request_id_ = 1;

  auto weak_ptr = weak_factory_.GetWeakPtr();

  // Attributes

  std::vector<scada::ReadValueId> read_ids;
  read_ids.reserve(nodes.size() * kFetchAttributesReserveFactor);
  for (auto* node : nodes) {
    logger_->WriteF(LogSeverity::Normal, "Fetching node %s with request %u",
                    ToString(node->node_id).c_str(), request_id);

    size_t count = read_ids.size();
    GetFetchAttributes(node->node_id, node->is_property, node->is_declaration,
                       read_ids);
    node->attributes_fetched = count == read_ids.size();
    node->fetch_request_id = request_id;
  }

  attribute_service_.Read(
      read_ids,
      [weak_ptr, request_id, start_ticks, read_ids](
          scada::Status&& status, std::vector<scada::DataValue>&& results) {
        if (auto* ptr = weak_ptr.get()) {
          ptr->OnReadResult(request_id, start_ticks, std::move(status),
                            read_ids, std::move(results));
        }
      });

  // References

  std::vector<scada::BrowseDescription> descriptions;
  descriptions.reserve(nodes.size() * kFetchReferencesReserveFactor);
  for (auto* node : nodes) {
    size_t count = descriptions.size();
    GetFetchReferences(*node, node->fetch_parent, descriptions);
    node->references_fetched = count == descriptions.size();
  }

  if (descriptions.empty()) {
    OnBrowseResult(request_id, start_ticks, scada::StatusCode::Good, {}, {});
    return;
  }

  view_service_.Browse(
      descriptions,
      [weak_ptr, request_id, start_ticks, descriptions](
          scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        if (auto* ptr = weak_ptr.get()) {
          ptr->OnBrowseResult(request_id, start_ticks, std::move(status),
                              std::move(descriptions), std::move(results));
        }
      });
}

void NodeFetcherImpl::NotifyFetchedNodes() {
  auto [nodes, errors] = fetching_nodes_.GetFetchedNodes();
  if (nodes.empty() && errors.empty())
    return;

  if (!errors.empty()) {
    for (auto& error : errors) {
      logger_->WriteF(LogSeverity::Warning, "Node %s fetch error %s",
                      NodeIdToScadaString(error.first).c_str(),
                      ToString(error.second).c_str());
    }
  }

  if (!nodes.empty()) {
    for (auto& node : nodes) {
      logger_->WriteF(
          LogSeverity::Normal,
          "Node fetched {node_id: '%s', node_class: '%s', parent_id: '%s', "
          "reference_type_id: '%s', browse_name: '%s', display_name: '%ls', "
          "data_type_id: '%s', value: '%ls'}",
          NodeIdToScadaString(node.node_id).c_str(),
          ToString(node.node_class).c_str(),
          NodeIdToScadaString(node.parent_id).c_str(),
          NodeIdToScadaString(node.reference_type_id).c_str(),
          ToString(node.attributes.browse_name).c_str(),
          ToString16(node.attributes.display_name).c_str(),
          NodeIdToScadaString(node.attributes.data_type).c_str(),
          ToString16(node.attributes.value).c_str());
    }
  }

  logger_->WriteF(LogSeverity::Normal,
                  "%Iu nodes completed, %Iu incomplete nodes remain",
                  nodes.size() + errors.size(), fetching_nodes_.size());

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
    assert(!node.pending);

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
  logger_->WriteF(LogSeverity::Normal,
                  "Read request %u completed in %u ms with status: %s",
                  request_id, static_cast<unsigned>(duration.InMilliseconds()),
                  ToString(status).c_str());

  for (size_t i = 0; i < read_ids.size(); ++i) {
    auto& read_id = read_ids[i];

    auto* node = fetching_nodes_.FindNode(read_id.node_id);
    if (!node)
      continue;

    // Request cancelation.
    if (!node->fetch_started || node->fetch_request_id != request_id) {
      logger_->WriteF(LogSeverity::Normal,
                      "Ignore read response for canceled node %s",
                      ToString(node->node_id).c_str());
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
  logger_->WriteF(LogSeverity::Normal, "%Iu requests are running",
                  running_request_count_);

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
  logger_->WriteF(LogSeverity::Normal,
                  "Browse request %u completed in %u ms with status: %s",
                  request_id, static_cast<unsigned>(duration.InMilliseconds()),
                  ToString(status).c_str());

  for (size_t i = 0; i < descriptions.size(); ++i) {
    auto& description = descriptions[i];

    auto* node = fetching_nodes_.FindNode(description.node_id);
    if (!node)
      continue;

    // Request cancelation.
    if (!node->fetch_started || node->fetch_request_id != request_id) {
      logger_->WriteF(LogSeverity::Normal,
                      "Ignore browse response for canceled node %s",
                      ToString(node->node_id).c_str());
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
  logger_->WriteF(LogSeverity::Normal, "%Iu requests are running",
                  running_request_count_);

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
    assert(description.reference_type_id == scada::id::HierarchicalReferences);

    node.reference_type_id = std::move(reference.reference_type_id);
    node.parent_id = std::move(reference.node_id);

    if (!node.parent_id.is_null())
      ValidateDependency(node, node.parent_id);

  } else if (description.reference_type_id == scada::id::Aggregates) {
    // Child.

    assert(reference.forward);

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
    }

    fetching_nodes_.AddDependency(child, node);
    if (description.reference_type_id == scada::id::Aggregates)
      fetching_nodes_.AddDependency(node, child);

    FetchNode(child);

  } else {
    // Non-hierarchical forward references, including HasTypeDefinition.

    assert(reference.forward);
    assert(description.reference_type_id ==
           scada::id::NonHierarchicalReferences);

    if (reference.reference_type_id == scada::id::HasTypeDefinition)
      node.type_definition_id = reference.node_id;
    else
      node.references.emplace_back(reference);

    if (reference.node_id != node.node_id)
      ValidateDependency(node, reference.node_id);
  }
}

bool NodeFetcherImpl::AssertValid() const {
  return true;

  for (auto* node : pending_queue_) {
    assert(node->pending);
    if (!node->pending)
      return false;
  }

  for (auto& p : fetching_nodes_.fetching_nodes_) {
    auto& node = p.second;

    if (node.fetched()) {
      assert(!node.pending);
      if (node.pending)
        return false;

      assert(node.fetch_started);
      if (!node.fetch_started)
        return false;

    } else {
      assert(node.pending || node.fetch_started);
      if (!node.pending && !node.fetch_started)
        return false;
    }

    if (node.pending) {
      assert(!node.fetched());
      if (node.fetched())
        return false;

      auto i = std::find(pending_queue_.begin(), pending_queue_.end(), &node);
      assert(i != pending_queue_.end());
      if (i == pending_queue_.end())
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
  assert(!from_id.is_null());
  if (!node_validator_(from_id)) {
    auto& from = fetching_nodes_.AddNode(from_id);
    // The equality is possible for references.
    if (&from != &node) {
      from.fetch_parent = true;
      from.force |= node.force;
      fetching_nodes_.AddDependency(node, from);
      FetchNode(from);
    }
  }
}
