#include "node_ref_service_impl.h"

#include <set>

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/logger.h"
#include "core/attribute_service.h"
#include "core/standard_node_ids.h"
#include "common/node_ref_impl.h"
#include "common/node_ref_observer.h"
#include "common/node_ref_util.h"

namespace {

const scada::AttributeId kReadAttributeIds[] = {
    OpcUa_Attributes_NodeClass,
    OpcUa_Attributes_BrowseName,
    OpcUa_Attributes_DisplayName,
    OpcUa_Attributes_DataType,
    OpcUa_Attributes_Value,
};

} // namespace

NodeRefServiceImpl::NodeRefServiceImpl(NodeRefServiceImplContext&& context)
    : NodeRefServiceImplContext(std::move(context)) {
  view_service_.Subscribe(*this);
}

NodeRefServiceImpl::~NodeRefServiceImpl() {
  view_service_.Unsubscribe(*this);
}

void NodeRefServiceImpl::RequestNode(const scada::NodeId& node_id, const RequestNodeCallback& callback) {
  GetPartialNode(node_id, callback, {});
}

NodeRef NodeRefServiceImpl::GetNode(const scada::NodeId& node_id) {
  return NodeRef{GetPartialNode(node_id, nullptr, {})};
}

std::shared_ptr<NodeRefImpl> NodeRefServiceImpl::GetPartialNode(const scada::NodeId& node_id, const RequestNodeCallback& callback,
    const scada::NodeId& depended_id) {
  {
    auto i = cached_nodes_.find(node_id);
    if (i != cached_nodes_.end()) {
      auto& node = i->second;
      if (callback)
        callback();
      return node;
    }
  }

  auto& partial_node = partial_nodes_[node_id];
  if (callback)
    partial_node.callbacks.emplace_back(callback);
  if (!depended_id.is_null())
    partial_node.depended_ids.emplace_back(depended_id);

  auto& impl = partial_node.impl;
  if (!impl) {
    assert(partial_node.pending_request_count == 0);

    impl = std::make_shared<NodeRefImpl>(*this, node_id);

    logger_->WriteF(LogSeverity::Normal, "Request node %s", node_id.ToString().c_str());

    std::vector<scada::ReadValueId> read_ids(std::size(kReadAttributeIds));
    for (size_t i = 0; i < read_ids.size(); ++i)
      read_ids[i] = scada::ReadValueId{node_id, kReadAttributeIds[i]};

    // TODO: Weak ptr.
    ++partial_node.pending_request_count;
    attribute_service_.Read(read_ids, [=](const scada::Status& status, std::vector<scada::DataValue> values) {
      OnReadComplete(node_id, status, std::move(values));
    });
  }

  return impl;
}

void NodeRefServiceImpl::SetAttribute(NodeRefImpl& impl, scada::AttributeId attribute_id, scada::DataValue data_value) {
  switch (attribute_id) {
    case OpcUa_Attributes_NodeClass:
      impl.node_class_ = static_cast<scada::NodeClass>(data_value.value.as_int32());
      break;
    case OpcUa_Attributes_BrowseName:
      impl.browse_name_ = data_value.value.as_string();
      assert(!impl.browse_name_.empty());
      break;
    case OpcUa_Attributes_DisplayName:
      impl.display_name_ = data_value.value.as_string16();
      assert(!impl.display_name_.empty());
      break;
    case OpcUa_Attributes_DataType:
      impl.data_type_ = GetPartialNode(data_value.value.as_node_id(), nullptr, impl.id_);
      break;
    case OpcUa_Attributes_Value:
      impl.data_value_ = std::move(data_value);
      break;
  }
}

void NodeRefServiceImpl::AddReference(NodeRefImpl& impl, const NodeRefImplReference& reference) {
//  assert(reference.reference_type.fetched());
//  assert(reference.target.fetched());

  if (reference.forward) {
    if (IsSubtypeOf(NodeRef{reference.reference_type}, OpcUaId_HasTypeDefinition))
      impl.type_definition_ = reference.target;
    else if (IsSubtypeOf(NodeRef{reference.reference_type}, OpcUaId_Aggregates))
      impl.aggregates_.push_back(reference);
    else if (IsSubtypeOf(NodeRef{reference.reference_type}, OpcUaId_NonHierarchicalReferences))
      impl.references_.push_back(reference);

  } else {
    if (IsSubtypeOf(NodeRef{reference.reference_type}, OpcUaId_HasSubtype))
      impl.supertype_ = reference.target;
  }
}

void NodeRefServiceImpl::OnReadComplete(const scada::NodeId& node_id, const scada::Status& status, std::vector<scada::DataValue> values) {
  if (!status) {
    logger_->WriteF(LogSeverity::Warning, "Read failed for node %s", node_id.ToString().c_str());
    // TODO: Failure.
    return;
  }

  assert(values.size() == std::size(kReadAttributeIds));

  logger_->WriteF(LogSeverity::Normal, "Read complete for node %s", node_id.ToString().c_str());

  auto i = partial_nodes_.find(node_id);
  if (i == partial_nodes_.end())
    return;

  auto& partial_node = i->second;
  auto& impl = partial_node.impl;

  assert(partial_node.pending_request_count > 0);
  --partial_node.pending_request_count;

  for (size_t i = 0; i < values.size(); ++i) {
    const auto attribute_id = kReadAttributeIds[i];
    auto& value = values[i];
    // assert(scada::IsGood(value.status_code) || attribute_id != OpcUa_Attributes_BrowseName);
    if (scada::IsGood(value.status_code))
      SetAttribute(*impl, attribute_id, std::move(value));
  }

  // TODO: Data integrity check.

  std::vector<scada::BrowseDescription> nodes;
  nodes.reserve(3);
  if (scada::IsTypeDefinition(impl->node_class_))
    nodes.push_back({node_id, scada::BrowseDirection::Inverse, OpcUaId_HasSubtype, true});
  nodes.push_back({node_id, scada::BrowseDirection::Forward, OpcUaId_Aggregates, true});
  nodes.push_back({node_id, scada::BrowseDirection::Forward, OpcUaId_NonHierarchicalReferences, true});

  ++partial_node.pending_request_count;
  view_service_.Browse(std::move(nodes),
      [=](const scada::Status& status, std::vector<scada::BrowseResult> results) {
        OnBrowseComplete(node_id, status, std::move(results));
      });
}

void NodeRefServiceImpl::OnBrowseComplete(const scada::NodeId& node_id, const scada::Status& status,
    std::vector<scada::BrowseResult> results) {
  logger_->WriteF(LogSeverity::Normal, "Browse complete for node %s", node_id.ToString().c_str());

  auto i = partial_nodes_.find(node_id);
  if (i == partial_nodes_.end())
    return;

  // TODO: Check |status|.

  auto& partial_node = i->second;
  auto& impl = partial_node.impl;

  assert(partial_node.pending_request_count > 0);
  --partial_node.pending_request_count;

  for (auto& result : results) {
    for (auto& reference : result.references) {
      partial_node.pending_references.push_back({
          GetPartialNode(reference.reference_type_id, nullptr, node_id),
          GetPartialNode(reference.node_id, nullptr, node_id),
          reference.forward,
      });
    }
  }

  // TODO: Integrity check.

  CompletePartialNode(node_id);
}

void NodeRefServiceImpl::CompletePartialNode(const scada::NodeId& node_id) {
  auto i = partial_nodes_.find(node_id);
  if (i == partial_nodes_.end())
    return;

  auto& partial_node = i->second;
  if (partial_node.pending_request_count != 0)
    return;

  auto& impl = partial_node.impl;

  std::vector<scada::NodeId> fetched_node_ids;
  IsNodeFetched(impl, fetched_node_ids);

  std::vector<scada::NodeId> all_dependent_ids;

  for (auto& fetched_id : fetched_node_ids) {
    auto i = partial_nodes_.find(fetched_id);
    if (i == partial_nodes_.end())
      continue;

    auto& partial_node = i->second;
    auto impl = partial_node.impl;
    auto callbacks = std::move(partial_node.callbacks);

    logger_->WriteF(LogSeverity::Normal, "Fetched node %s: %s", fetched_id.ToString().c_str(),
        impl->browse_name_.c_str());

    std::copy(partial_node.depended_ids.begin(), partial_node.depended_ids.end(),
      std::back_inserter(all_dependent_ids));

    // Init references.
    for (auto& reference : partial_node.pending_references)
      AddReference(*impl, reference);

    partial_nodes_.erase(i);

    cached_nodes_.emplace(impl->id_, impl);

    for (auto& o : observers_)
      o.OnNodeSemanticChanged(fetched_id);

    if (auto* node_observers = GetNodeObservers(fetched_id)) {
      for (auto& o : *node_observers)
        o.OnNodeSemanticChanged(fetched_id);
    }

    for (auto& callback : callbacks)
      callback();
  }

  for (auto& depended_id : all_dependent_ids)
    CompletePartialNode(depended_id);
}

bool NodeRefServiceImpl::IsNodeFetched(const std::shared_ptr<NodeRefImpl>& impl, std::vector<scada::NodeId>& fetched_node_ids) {
  if (!impl)
    return true;

  if (impl->fetched_)
    return true;

  auto i = partial_nodes_.find(impl->id_);
  // Must be either fetched or partial.
  assert(i != partial_nodes_.end());
  if (i == partial_nodes_.end())
    return true;

  auto& partial_node = i->second;
  if (partial_node.passing)
    return true;
  if (partial_node.pending_request_count != 0)
    return false;

  partial_node.passing = true;

  // It must never delete |partial_node| since it's in |complete_ids|.
  impl->fetched_ = IsNodeFetchedHelper(partial_node, fetched_node_ids);

  assert(partial_node.passing);
  partial_node.passing = false;

  if (impl->fetched_)
    fetched_node_ids.emplace_back(impl->id_);

  return impl->fetched_;
}

bool NodeRefServiceImpl::IsNodeFetchedHelper(PartialNode& partial_node, std::vector<scada::NodeId>& fetched_node_ids) {
  if (auto data_type = partial_node.impl->data_type_) {
    if (!IsNodeFetched(data_type, fetched_node_ids))
      return false;
  }

  for (auto& reference : partial_node.pending_references) {
    if (!IsNodeFetched(reference.reference_type, fetched_node_ids))
      return false;
    if (!IsNodeFetched(reference.target, fetched_node_ids))
      return false;
  }
  return true;
}

void NodeRefServiceImpl::Browse(const scada::BrowseDescription& description, const BrowseCallback& callback) {
  // TODO: Cache.

  view_service_.Browse({description}, [callback](const scada::Status& status, std::vector<scada::BrowseResult> results) {
    if (status)
      callback(status, std::move(results.front().references));
    else
      callback(status, {});
  });
}

void NodeRefServiceImpl::AddObserver(NodeRefObserver& observer) {
  observers_.AddObserver(&observer);
}

void NodeRefServiceImpl::RemoveObserver(NodeRefObserver& observer) {
  observers_.RemoveObserver(&observer);
}

void NodeRefServiceImpl::AddNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer) {
  node_observers_[node_id].AddObserver(&observer);
}

void NodeRefServiceImpl::RemoveNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer) {
  node_observers_[node_id].RemoveObserver(&observer);
}

void NodeRefServiceImpl::OnNodeAdded(const scada::NodeId& node_id) {
  if (auto* node_observers = GetNodeObservers(node_id)) {
    for (auto& o : *node_observers)
      o.OnNodeAdded(node_id);
  }

  for (auto& o : observers_)
    o.OnNodeAdded(node_id);
}

void NodeRefServiceImpl::OnNodeDeleted(const scada::NodeId& node_id) {
  // TODO: Remove nodes containing aggregates.
  cached_nodes_.erase(node_id);

  if (auto* node_observers = GetNodeObservers(node_id)) {
    for (auto& o : *node_observers)
      o.OnNodeDeleted(node_id);
  }

  for (auto& o : observers_)
    o.OnNodeDeleted(node_id);
}

void NodeRefServiceImpl::OnReferenceAdded(const scada::BrowseReference& reference) {
  // TODO: Append aggregates and types.
  if (auto* node_observers = GetNodeObservers(reference.source_id)) {
    for (auto& o : *node_observers)
      o.OnReferenceAdded(reference.source_id);
  }
  if (auto* node_observers = GetNodeObservers(reference.target_id)) {
    for (auto& o : *node_observers)
      o.OnReferenceAdded(reference.target_id);
  }

  for (auto& o : observers_)
    o.OnReferenceAdded(reference.source_id);
  for (auto& o : observers_)
    o.OnReferenceAdded(reference.target_id);
}

void NodeRefServiceImpl::OnReferenceDeleted(const scada::BrowseReference& reference) {
  // TODO: Delete aggregates and types.

  if (auto* node_observers = GetNodeObservers(reference.source_id)) {
    for (auto& o : *node_observers)
      o.OnReferenceDeleted(reference.source_id);
  }
  if (auto* node_observers = GetNodeObservers(reference.target_id)) {
    for (auto& o : *node_observers)
      o.OnReferenceDeleted(reference.target_id);
  }

  for (auto& o : observers_)
    o.OnReferenceDeleted(reference.source_id);
  for (auto& o : observers_)
    o.OnReferenceDeleted(reference.target_id);
}

void NodeRefServiceImpl::OnNodeModified(const scada::NodeId& node_id, const scada::PropertyIds& property_ids) {
  if (auto* node_observers = GetNodeObservers(node_id)) {
    for (auto& o : *node_observers)
      o.OnNodeDeleted(node_id);
  }

  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id);
}

const NodeRefServiceImpl::Observers* NodeRefServiceImpl::GetNodeObservers(const scada::NodeId& node_id) const {
  auto i = node_observers_.find(node_id);
  return i != node_observers_.end() ? &i->second : nullptr;
}
