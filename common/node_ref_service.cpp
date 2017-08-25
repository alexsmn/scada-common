#include "node_ref_service.h"

#include <set>

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/logger.h"
#include "core/attribute_service.h"
#include "core/standard_node_ids.h"
#include "common/node_ref_data.h"
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

NodeRefService::NodeRefService(NodeRefServiceContext&& context)
    : NodeRefServiceContext(std::move(context)) {
  view_service_.Subscribe(*this);
}

NodeRefService::~NodeRefService() {
  view_service_.Unsubscribe(*this);
}

NodeRef NodeRefService::GetCachedNode(const scada::NodeId& node_id) const {
  return node_cache_.Find(node_id);
}

void NodeRefService::RequestNode(const scada::NodeId& node_id, const RequestNodeCallback& callback) {
  GetPartialNode(node_id, callback, {});
}

NodeRef NodeRefService::GetPartialNode(const scada::NodeId& node_id) {
  return GetPartialNode(node_id, nullptr, {});
}

NodeRef NodeRefService::GetPartialNode(const scada::NodeId& node_id, const RequestNodeCallback& callback,
    const scada::NodeId& depended_id) {
  if (auto node = node_cache_.Find(node_id)) {
    if (callback)
      callback(scada::StatusCode::Good, node);
    return node;
  }

  auto& partial_node = partial_nodes_[node_id];
  if (callback)
    partial_node.callbacks.emplace_back(callback);
  if (!depended_id.is_null())
    partial_node.depended_ids.emplace_back(depended_id);

  auto& node = partial_node.node;
  if (!node) {
    assert(partial_node.pending_request_count == 0);

    node = NodeRef{std::make_shared<NodeRefData>(NodeRefData{node_id, *this})};

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

  return node;
}

void NodeRefService::SetAttribute(NodeRefData& data, scada::AttributeId attribute_id, scada::DataValue data_value) {
  switch (attribute_id) {
    case OpcUa_Attributes_NodeClass:
      data.node_class = static_cast<scada::NodeClass>(data_value.value.as_int32());
      break;
    case OpcUa_Attributes_BrowseName:
      data.browse_name = data_value.value.as_string();
      assert(!data.browse_name.empty());
      break;
    case OpcUa_Attributes_DisplayName:
      data.display_name = data_value.value.as_string16();
      assert(!data.display_name.empty());
      break;
    case OpcUa_Attributes_DataType:
      data.data_type = GetPartialNode(data_value.value.as_node_id(), nullptr, data.id);
      break;
    case OpcUa_Attributes_Value:
      data.data_value = data_value;
      break;
  }
}

void NodeRefService::AddReference(NodeRefData& data, const NodeRef::Reference& reference) {
//  assert(reference.reference_type.fetched());
//  assert(reference.target.fetched());

  if (reference.forward) {
    if (IsSubtypeOf(reference.reference_type, OpcUaId_HasTypeDefinition))
      data.type_definition = reference.target;
    else if (IsSubtypeOf(reference.reference_type, OpcUaId_Aggregates))
      data.aggregates.push_back(reference);
    else if (IsSubtypeOf(reference.reference_type, OpcUaId_NonHierarchicalReferences))
      data.references.push_back(reference);

  } else {
    if (IsSubtypeOf(reference.reference_type, OpcUaId_HasSubtype))
      data.supertype = reference.target;
  }
}

void NodeRefService::OnReadComplete(const scada::NodeId& node_id, const scada::Status& status, std::vector<scada::DataValue> values) {
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
  auto& data = partial_node.node.data_;

  assert(partial_node.pending_request_count > 0);
  --partial_node.pending_request_count;

  for (size_t i = 0; i < values.size(); ++i) {
    const auto attribute_id = kReadAttributeIds[i];
    auto& value = values[i];
    // assert(scada::IsGood(value.status_code) || attribute_id != OpcUa_Attributes_BrowseName);
    if (scada::IsGood(value.status_code))
      SetAttribute(*data, attribute_id, std::move(value));
  }

  // TODO: Data integrity check.

  std::vector<scada::BrowseDescription> nodes;
  nodes.reserve(3);
  if (scada::IsTypeDefinition(data->node_class))
    nodes.push_back({node_id, scada::BrowseDirection::Inverse, OpcUaId_HasSubtype, true});
  nodes.push_back({node_id, scada::BrowseDirection::Forward, OpcUaId_Aggregates, true});
  nodes.push_back({node_id, scada::BrowseDirection::Forward, OpcUaId_NonHierarchicalReferences, true});

  ++partial_node.pending_request_count;
  view_service_.Browse(std::move(nodes),
      [=](const scada::Status& status, std::vector<scada::BrowseResult> results) {
        OnBrowseComplete(node_id, status, std::move(results));
      });
}

void NodeRefService::OnBrowseComplete(const scada::NodeId& node_id, const scada::Status& status,
    std::vector<scada::BrowseResult> results) {
  logger_->WriteF(LogSeverity::Normal, "Browse complete for node %s", node_id.ToString().c_str());

  auto i = partial_nodes_.find(node_id);
  if (i == partial_nodes_.end())
    return;

  // TODO: Check |status|.

  auto& partial_node = i->second;
  auto& data = partial_node.node.data_;

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

void NodeRefService::CompletePartialNode(const scada::NodeId& node_id) {
  auto i = partial_nodes_.find(node_id);
  if (i == partial_nodes_.end())
    return;

  auto& partial_node = i->second;
  if (partial_node.pending_request_count != 0)
    return;

  auto& data = partial_node.node.data_;

  std::vector<scada::NodeId> fetched_node_ids;
  IsNodeFetched(data, fetched_node_ids);

  std::vector<scada::NodeId> all_dependent_ids;

  for (auto& fetched_id : fetched_node_ids) {
    auto i = partial_nodes_.find(fetched_id);
    if (i == partial_nodes_.end())
      continue;

    auto& partial_node = i->second;
    auto node = partial_node.node;
    auto callbacks = std::move(partial_node.callbacks);

    logger_->WriteF(LogSeverity::Normal, "Fetched node %s: %s", fetched_id.ToString().c_str(),
        node.browse_name().c_str());

    std::copy(partial_node.depended_ids.begin(), partial_node.depended_ids.end(),
      std::back_inserter(all_dependent_ids));

    // Init references.
    for (auto& reference : partial_node.pending_references)
      AddReference(*node.data_, reference);

    partial_nodes_.erase(i);

    node_cache_.Add(node);

    for (auto& o : observers_)
      o.OnNodeSemanticChanged(fetched_id);

    if (auto* node_observers = GetNodeObservers(fetched_id)) {
      for (auto& o : *node_observers)
        o.OnNodeSemanticChanged(fetched_id);
    }

    for (auto& callback : callbacks)
      callback(scada::StatusCode::Good, node);
  }

  for (auto& depended_id : all_dependent_ids)
    CompletePartialNode(depended_id);
}

bool NodeRefService::IsNodeFetched(std::shared_ptr<NodeRefData> data, std::vector<scada::NodeId>& fetched_node_ids) {
  if (!data)
    return true;

  if (data->fetched)
    return true;

  auto i = partial_nodes_.find(data->id);
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
  data->fetched = IsNodeFetchedHelper(partial_node, fetched_node_ids);

  assert(partial_node.passing);
  partial_node.passing = false;

  if (data->fetched)
    fetched_node_ids.emplace_back(data->id);

  return data->fetched;
}

bool NodeRefService::IsNodeFetchedHelper(PartialNode& partial_node, std::vector<scada::NodeId>& fetched_node_ids) {
  if (auto data_type = partial_node.node.data_->data_type) {
    if (!IsNodeFetched(data_type.data_, fetched_node_ids))
      return false;
  }

  for (auto& reference : partial_node.pending_references) {
    if (!IsNodeFetched(reference.reference_type.data_, fetched_node_ids))
      return false;
    if (!IsNodeFetched(reference.target.data_, fetched_node_ids))
      return false;
  }
  return true;
}

void NodeRefService::Browse(const scada::BrowseDescription& description, const BrowseCallback& callback) {
  // TODO: Cache.

  view_service_.Browse({description}, [callback](const scada::Status& status, std::vector<scada::BrowseResult> results) {
    if (status)
      callback(status, std::move(results.front().references));
    else
      callback(status, {});
  });
}

void NodeRefService::AddObserver(NodeRefObserver& observer) {
  observers_.AddObserver(&observer);
}

void NodeRefService::RemoveObserver(NodeRefObserver& observer) {
  observers_.RemoveObserver(&observer);
}

void NodeRefService::AddNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer) {
  node_observers_[node_id].AddObserver(&observer);
}

void NodeRefService::RemoveNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer) {
  node_observers_[node_id].RemoveObserver(&observer);
}

void NodeRefService::OnNodeAdded(const scada::NodeId& node_id) {
  if (auto* node_observers = GetNodeObservers(node_id)) {
    for (auto& o : *node_observers)
      o.OnNodeAdded(node_id);
  }

  for (auto& o : observers_)
    o.OnNodeAdded(node_id);
}

void NodeRefService::OnNodeDeleted(const scada::NodeId& node_id) {
  // TODO: Remove nodes containing aggregates.
  node_cache_.Remove(node_id);

  if (auto* node_observers = GetNodeObservers(node_id)) {
    for (auto& o : *node_observers)
      o.OnNodeDeleted(node_id);
  }

  for (auto& o : observers_)
    o.OnNodeDeleted(node_id);
}

void NodeRefService::OnReferenceAdded(const scada::BrowseReference& reference) {
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

void NodeRefService::OnReferenceDeleted(const scada::BrowseReference& reference) {
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

void NodeRefService::OnNodeModified(const scada::NodeId& node_id, const scada::PropertyIds& property_ids) {
  if (auto* node_observers = GetNodeObservers(node_id)) {
    for (auto& o : *node_observers)
      o.OnNodeDeleted(node_id);
  }

  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id);
}

const NodeRefService::Observers* NodeRefService::GetNodeObservers(const scada::NodeId& node_id) const {
  auto i = node_observers_.find(node_id);
  return i != node_observers_.end() ? &i->second : nullptr;
}
