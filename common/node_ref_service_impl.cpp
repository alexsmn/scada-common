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

NodeRefServiceImpl::NodeRefServiceImpl(NodeRefServiceImplContext&& context)
    : NodeRefServiceImplContext(std::move(context)) {
  view_service_.Subscribe(*this);
}

NodeRefServiceImpl::~NodeRefServiceImpl() {
  view_service_.Unsubscribe(*this);
}

NodeRef NodeRefServiceImpl::GetNode(const scada::NodeId& node_id) {
  return NodeRef{GetNodeImpl(node_id, {})};
}

std::shared_ptr<NodeRefImpl> NodeRefServiceImpl::GetNodeImpl(const scada::NodeId& node_id, const scada::NodeId& depended_id) {
  assert(!node_id.is_null());

  auto& impl = nodes_[node_id];

  if (!impl) {
    impl = std::make_shared<NodeRefImpl>(*this, node_id, logger_);
    impl->Fetch(nullptr);
  }

  if (!impl->fetched_ && !depended_id.is_null())
    impl->depended_ids_.emplace_back(depended_id);

  return impl;
}

void NodeRefServiceImpl::CompletePartialNode(const scada::NodeId& node_id) {
  auto i = nodes_.find(node_id);
  if (i == nodes_.end())
    return;

  auto& impl = i->second;
  assert(!impl->fetched_);

  std::vector<scada::NodeId> fetched_node_ids;
  impl->IsNodeFetched(fetched_node_ids);

  std::vector<scada::NodeId> all_dependent_ids;

  for (auto& fetched_id : fetched_node_ids) {
    auto i = nodes_.find(fetched_id);
    if (i == nodes_.end())
      continue;

    auto impl = i->second;
    assert(!impl->fetched_);
    auto callbacks = std::move(impl->fetch_callbacks_);

    logger_->WriteF(LogSeverity::Normal, "Fetched node %s: %s", fetched_id.ToString().c_str(),
        impl->browse_name_.name().c_str());

    std::copy(impl->depended_ids_.begin(), impl->depended_ids_.end(),
      std::back_inserter(all_dependent_ids));

    // Init references.
    for (auto& reference : impl->references_)
      impl->AddReference(reference);

    impl->fetched_ = true;

    for (auto& o : observers_)
      o.OnNodeSemanticChanged(fetched_id);

    if (auto* node_observers = GetNodeObservers(fetched_id)) {
      for (auto& o : *node_observers)
        o.OnNodeSemanticChanged(fetched_id);
    }

    for (auto& callback : callbacks)
      callback(impl);
  }

  for (auto& depended_id : all_dependent_ids)
    CompletePartialNode(depended_id);
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
  nodes_.erase(node_id);

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
