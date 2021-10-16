#include "address_space/address_space_impl.h"

#include "address_space/address_space_util.h"
#include "address_space/node_observer.h"
#include "address_space/object.h"
#include "address_space/reference.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "core/attribute_ids.h"
#include "core/standard_node_ids.h"
#include "core/status.h"

AddressSpaceImpl::AddressSpaceImpl(scada::AddressSpace* parent_address_space)
    : parent_address_space_{parent_address_space} {}

AddressSpaceImpl::~AddressSpaceImpl() {
  Clear();
}

void AddressSpaceImpl::Clear() {
  for (auto& p : node_map_)
    DeleteAllReferences(*p.second);
  node_map_.clear();
}

bool AddressSpaceImpl::ModifyNode(const scada::NodeId& id,
                                  scada::NodeAttributes attributes,
                                  scada::NodeProperties properties) {
  auto* node = GetMutableNode(id);
  assert(node);

  scada::AttributeSet attribute_set;

  if (!attributes.browse_name.empty() &&
      node->GetBrowseName() != attributes.browse_name) {
    attribute_set.Add(scada::AttributeId::BrowseName);
    node->SetBrowseName(std::move(attributes.browse_name));
  }

  if (!attributes.display_name.empty() &&
      node->GetDisplayName() != attributes.display_name) {
    attribute_set.Add(scada::AttributeId::DisplayName);
    node->SetDisplayName(std::move(attributes.display_name));
  }

  if (attributes.value.has_value()) {
    if (auto* variable = scada::AsVariable(node)) {
      scada::DataValue new_data_value{std::move(*attributes.value), {}, {}, {}};
      if (variable->GetValue() != new_data_value) {
        attribute_set.Add(scada::AttributeId::Value);

        // Property ignores timestamps.
        // TODO: Avoid timestamp.
        auto status = variable->SetValue(std::move(new_data_value));
        if (!status) {
          // TODO: Handle error.
          assert(false);
        }
      }

    } else {
      // TODO: Handle error.
      assert(false);
    }
  }

  // Properties.
  scada::NodeId pids[50];
  size_t pid_count = 0;

  for (auto& [prop_decl_id, value] : properties) {
    auto old_value = scada::GetPropertyValue(*node, prop_decl_id);
    if (value == old_value)
      continue;

    pids[pid_count++] = prop_decl_id;
    auto status =
        scada::SetPropertyValue(*node, prop_decl_id, std::move(value));
    if (!status) {
      // TODO: Handle error.
      assert(false);
    }
  }

  // Notify.

  if (attribute_set.empty() && pid_count == 0)
    return false;

  scada::PropertyIds property_ids(pid_count, pids);
  node->OnNodeModified(attribute_set, property_ids);
  NotifyNodeModified(*node, property_ids);

  return true;
}

void AddressSpaceImpl::AddNode(scada::Node& node) {
  assert(!node.id().is_null());
  assert(!GetNode(node.id()));

  auto& mapped_node = node_map_[node.id()];
  assert(!mapped_node);
  mapped_node = &node;
}

void AddNodeAndReference(AddressSpaceImpl& address_space,
                         scada::Node& node,
                         const scada::NodeId& reference_type_id,
                         const scada::NodeId& parent_id) {
  auto* parent = address_space.GetMutableNode(parent_id);
  assert(parent);

  AddNodeAndReference(address_space, node, reference_type_id, *parent);
}

void AddNodeAndReference(AddressSpaceImpl& address_space,
                         scada::Node& node,
                         const scada::NodeId& reference_type_id,
                         scada::Node& parent) {
  auto node_id = node.id();
  address_space.AddNode(node);
  AddReference(address_space, reference_type_id, parent, node);
}

void AddressSpaceImpl::RemoveNode(const scada::NodeId& id) {
  auto i = node_map_.find(id);
  if (i == node_map_.end())
    return;

  auto*& node = i->second;

  for (;;) {
    auto children = scada::GetChildren(*node);
    if (children.empty())
      break;
    auto& child = *children.front();
    RemoveNode(child.id());
  }

  DeleteAllReferences(*node);

  NotifyNodeDeleted(*node);

  node_events_.erase(id);

  node_map_.erase(i);

  static_nodes_.erase(id);
}

scada::Node* AddressSpaceImpl::GetMutableNode(const scada::NodeId& node_id) {
  auto i = node_map_.find(node_id);
  if (i != node_map_.end())
    return i->second;

  if (parent_address_space_) {
    if (auto* node = parent_address_space_->GetMutableNode(node_id))
      return node;
  }

  return nullptr;
}

const scada::Node* AddressSpaceImpl::GetNode(
    const scada::NodeId& node_id) const {
  auto i = node_map_.find(node_id);
  if (i != node_map_.end())
    return i->second;

  if (parent_address_space_) {
    if (auto* node = parent_address_space_->GetNode(node_id))
      return node;
  }

  return nullptr;
}

void AddressSpaceImpl::Subscribe(scada::NodeObserver& events) const {
  observers_.AddObserver(&events);

  if (parent_address_space_)
    parent_address_space_->Subscribe(events);
}

void AddressSpaceImpl::Unsubscribe(scada::NodeObserver& events) const {
  assert(observers_.HasObserver(&events));
  observers_.RemoveObserver(&events);

  if (parent_address_space_)
    parent_address_space_->Unsubscribe(events);
}

void AddressSpaceImpl::SubscribeNode(const scada::NodeId& node_id,
                                     scada::NodeObserver& events) const {
  node_events_[node_id].AddObserver(&events);

  if (parent_address_space_)
    parent_address_space_->SubscribeNode(node_id, events);
}

void AddressSpaceImpl::UnsubscribeNode(const scada::NodeId& node_id,
                                       scada::NodeObserver& events) const {
  auto i = node_events_.find(node_id);
  assert(i != node_events_.end());
  auto& e = i->second;
  e.RemoveObserver(&events);

  if (parent_address_space_)
    parent_address_space_->UnsubscribeNode(node_id, events);
}

void AddressSpaceImpl::NotifyNodeAdded(const scada::Node& node) const {
  for (auto* parent = &node; parent; parent = GetParent(*parent)) {
    if (auto* events = GetNodeEvents(node.id())) {
      for (auto& o : *events)
        o.OnNodeCreated(node);
    }
  }

  for (auto& o : observers_)
    o.OnNodeCreated(node);
}

void AddressSpaceImpl::NotifyNodeDeleted(const scada::Node& node) const {
  for (auto* parent = &node; parent; parent = GetParent(*parent)) {
    if (auto* events = GetNodeEvents(node.id())) {
      for (auto& o : *events)
        o.OnNodeDeleted(node);
    }
  }

  for (auto& o : observers_)
    o.OnNodeDeleted(node);
}

void AddressSpaceImpl::NotifyNodeMoved(const scada::Node& node,
                                       const scada::Node* top) const {
  for (auto* parent = &node; parent && parent != top;
       parent = GetParent(*parent)) {
    if (auto* events = GetNodeEvents(node.id())) {
      for (auto& o : *events)
        o.OnNodeMoved(node);
    }
  }

  for (auto& o : observers_)
    o.OnNodeMoved(node);
}

void AddressSpaceImpl::NotifyNodeTitleChanged(const scada::Node& node) const {
  for (auto* parent = &node; parent; parent = GetParent(*parent)) {
    if (auto* events = GetNodeEvents(node.id())) {
      for (auto& o : *events)
        o.OnNodeTitleChanged(node);
    }
  }

  for (auto& o : observers_)
    o.OnNodeTitleChanged(node);
}

void AddressSpaceImpl::NotifyNodeModified(
    const scada::Node& node,
    const scada::PropertyIds& property_ids) const {
  for (auto* parent = &node; parent; parent = GetParent(*parent)) {
    if (auto* events = GetNodeEvents(node.id())) {
      for (auto& o : *events)
        o.OnNodeModified(node, property_ids);
    }
  }

  for (auto& o : observers_)
    o.OnNodeModified(node, property_ids);
}

void AddressSpaceImpl::NotifyReference(
    const scada::ReferenceType& reference_type,
    const scada::Node& source,
    const scada::Node& target,
    bool added) const {
  for (auto& o : observers_) {
    if (added)
      o.OnReferenceAdded(reference_type, source, target);
    else
      o.OnReferenceDeleted(reference_type, source, target);
  }

  if (scada::IsSubtypeOf(reference_type, scada::id::HierarchicalReferences))
    NotifyNodeMoved(target, nullptr);
}

const AddressSpaceImpl::NodeEvents* AddressSpaceImpl::GetNodeEvents(
    const scada::NodeId& node_id) const {
  auto i = node_events_.find(node_id);
  return i == node_events_.end() ? nullptr : &i->second;
}

void AddressSpaceImpl::AddStaticNodeHelper(std::unique_ptr<scada::Node> node) {
  AddNode(*node);
  static_nodes_.emplace(node->id(), std::move(node));
}
