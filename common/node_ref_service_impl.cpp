#include "node_ref_service_impl.h"

#include <set>

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/logger.h"
#include "core/attribute_service.h"
#include "core/standard_node_ids.h"
#include "common/node_model_impl.h"
#include "common/node_ref_observer.h"
#include "common/node_ref_util.h"

NodeRefServiceImpl::NodeRefServiceImpl(NodeRefServiceImplContext&& context)
    : NodeRefServiceImplContext(std::move(context)) {
  view_service_.Subscribe(*this);
}

NodeRefServiceImpl::~NodeRefServiceImpl() {
  view_service_.Unsubscribe(*this);
}

NodeRef NodeRefServiceImpl::GetNode(const scada::NodeId& node_id) {
  if (node_id.is_null())
    return {};

  return GetNodeImpl(node_id, {});
}

std::shared_ptr<NodeModelImpl> NodeRefServiceImpl::GetNodeImpl(const scada::NodeId& node_id, const scada::NodeId& depended_id) {
  assert(!node_id.is_null());

  auto& node = nodes_[node_id];

  if (!node) {
    node = std::make_shared<NodeModelImpl>(*this, node_id, logger_);
    node->Fetch(nullptr);
  }

  if (!node->fetched_ && !depended_id.is_null())
    node->depended_ids_.emplace_back(depended_id);

  return node;
}

void NodeRefServiceImpl::CompletePartialNode(const std::shared_ptr<NodeModelImpl>& node) {
  if (node->fetched_)
    return;

  std::vector<scada::NodeId> fetched_node_ids;
  node->IsNodeFetched(fetched_node_ids);

  std::vector<scada::NodeId> all_dependent_ids;

  for (auto& fetched_id : fetched_node_ids) {
    auto i = nodes_.find(fetched_id);
    if (i == nodes_.end())
      continue;

    auto node = i->second;
    assert(!node->fetched_);
    assert(node->pending_request_count_ == 0);
    assert(!node->status_ || node->node_class_.has_value());
    assert(!node->status_ || !node->browse_name_.empty());

    auto callbacks = std::move(node->fetch_callbacks_);

    logger_->WriteF(LogSeverity::Normal, "Fetched node %s: %s", fetched_id.ToString().c_str(),
        node->browse_name_.name().c_str());

    std::copy(node->depended_ids_.begin(), node->depended_ids_.end(),
      std::back_inserter(all_dependent_ids));

    // Init references.
    for (auto& reference : node->pending_references_)
      node->AddReference(reference);
    node->pending_references_.clear();
    node->pending_references_.shrink_to_fit();

    node->fetched_ = true;

    for (auto& o : observers_)
      o.OnNodeSemanticChanged(fetched_id);

    if (auto* node_observers = GetNodeObservers(fetched_id)) {
      for (auto& o : *node_observers)
        o.OnNodeSemanticChanged(fetched_id);
    }

    for (auto& callback : callbacks)
      callback(node);
  }

  for (auto& depended_id : all_dependent_ids) {
    auto i = nodes_.find(depended_id);
    if (i != nodes_.end())
      CompletePartialNode(i->second);
  }
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

void NodeRefServiceImpl::NotifyEvent(const ModelChangeEvent& event) {
  if (auto* node_observers = GetNodeObservers(event.node_id)) {
    for (auto& o : *node_observers)
      o.OnModelChange(event);
  }

  for (auto& o : observers_)
    o.OnModelChange(event);
}

void NodeRefServiceImpl::OnNodeAdded(const scada::NodeId& node_id) {
  NotifyEvent({node_id, {}, ModelChangeEvent::NodeAdded});
}

void NodeRefServiceImpl::OnNodeDeleted(const scada::NodeId& node_id) {
  // TODO: Remove nodes containing aggregates.
  nodes_.erase(node_id);

  NotifyEvent({node_id, {}, ModelChangeEvent::NodeDeleted});
}

void NodeRefServiceImpl::OnReferenceAdded(const scada::NodeId& node_id) {
  NotifyEvent({node_id, {}, ModelChangeEvent::ReferenceAdded});
}

void NodeRefServiceImpl::OnReferenceDeleted(const scada::NodeId& node_id) {
  // TODO: Delete aggregates and types.

  NotifyEvent({node_id, {}, ModelChangeEvent::ReferenceDeleted});
}

void NodeRefServiceImpl::OnNodeSemanticsChanged(const scada::NodeId& node_id) {
  if (auto* node_observers = GetNodeObservers(node_id)) {
    for (auto& o : *node_observers)
      o.OnNodeSemanticChanged(node_id);
  }

  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id);
}

const NodeRefServiceImpl::Observers* NodeRefServiceImpl::GetNodeObservers(const scada::NodeId& node_id) const {
  auto i = node_observers_.find(node_id);
  return i != node_observers_.end() ? &i->second : nullptr;
}
