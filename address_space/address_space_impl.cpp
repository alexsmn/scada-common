#include "address_space/address_space_impl.h"

#include "address_space/node_observer.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "address_space/reference.h"
#include "address_space/scada_address_space.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "core/attribute_ids.h"
#include "core/standard_node_ids.h"
#include "core/status.h"

AddressSpaceImpl::AddressSpaceImpl(std::shared_ptr<Logger> logger)
    : logger_(std::move(logger)) {}

AddressSpaceImpl2::AddressSpaceImpl2(std::shared_ptr<Logger> logger)
    : AddressSpaceImpl{std::move(logger)},
      standard_address_space_(std::make_unique<StandardAddressSpace>()),
      static_address_space_(
          std::make_unique<StaticAddressSpace>(*standard_address_space_)) {
  AddNode(standard_address_space_->RootFolder);
  AddNode(standard_address_space_->ObjectsFolder);
  AddNode(standard_address_space_->TypesFolder);

  AddNode(standard_address_space_->References);
  AddNode(standard_address_space_->HierarchicalReference);
  AddNode(standard_address_space_->NonHierarchicalReference);
  AddNode(standard_address_space_->Aggregates);
  AddNode(standard_address_space_->HasProperty);
  AddNode(standard_address_space_->HasComponent);
  AddNode(standard_address_space_->HasSubtype);
  AddNode(standard_address_space_->HasTypeDefinition);
  AddNode(standard_address_space_->HasModellingRule);
  AddNode(standard_address_space_->Organizes);

  AddNode(standard_address_space_->BaseDataType);
  AddNode(standard_address_space_->BoolDataType);
  AddNode(standard_address_space_->IntDataType);
  AddNode(standard_address_space_->DoubleDataType);
  AddNode(standard_address_space_->StringDataType);
  AddNode(standard_address_space_->LocalizedTextDataType);
  AddNode(standard_address_space_->NodeIdDataType);

  AddNode(standard_address_space_->BaseObjectType);
  AddNode(standard_address_space_->FolderType);

  AddNode(standard_address_space_->BaseVariableType);
  AddNode(standard_address_space_->PropertyType);

  AddNode(standard_address_space_->ModellingRules);
  AddNode(standard_address_space_->ModellingRule_Mandatory);

  AddNode(static_address_space_->Creates);
  AddNode(static_address_space_->DeviceType);
  AddNode(static_address_space_->DeviceType.Disabled);
  AddNode(static_address_space_->DeviceType.Enabled);
  AddNode(static_address_space_->DeviceType.Online);
}

AddressSpaceImpl2::~AddressSpaceImpl2() {
  Clear();
}

AddressSpaceImpl::~AddressSpaceImpl() {
  Clear();
}

void AddressSpaceImpl::Clear() {
  for (auto& p : node_map_)
    DeleteAllReferences(*p.second);
  node_map_.clear();
}

void AddressSpaceImpl::ModifyNode(const scada::NodeId& id,
                                  scada::NodeAttributes attributes,
                                  scada::NodeProperties properties) {
  auto* node = GetNode(id);
  assert(node);

  scada::AttributeSet attribute_set;
  if (!attributes.browse_name.empty()) {
    attribute_set.Add(scada::AttributeId::BrowseName);
    node->SetBrowseName(std::move(attributes.browse_name));
  }
  if (!attributes.display_name.empty()) {
    attribute_set.Add(scada::AttributeId::DisplayName);
    node->SetDisplayName(std::move(attributes.display_name));
  }

  // Properties.
  {
    scada::NodeId pids[50];
    size_t pid_count = 0;

    for (auto& p : properties) {
      pids[pid_count++] = p.first;
      scada::SetPropertyValue(*node, p.first, std::move(p.second));
    }

    scada::PropertyIds property_ids(pid_count, pids);
    node->OnNodeModified(attribute_set, property_ids);
    NotifyNodeModified(*node, property_ids);
  }
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
  auto* parent = address_space.GetNode(parent_id);
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

scada::Node* AddressSpaceImpl::GetNode(const scada::NodeId& node_id) {
  auto i = node_map_.find(node_id);
  return i != node_map_.end() ? i->second : nullptr;
}

void AddressSpaceImpl::Subscribe(scada::NodeObserver& events) const {
  observers_.AddObserver(&events);
}

void AddressSpaceImpl::Unsubscribe(scada::NodeObserver& events) const {
  assert(observers_.HasObserver(&events));
  observers_.RemoveObserver(&events);
}

void AddressSpaceImpl::SubscribeNode(const scada::NodeId& node_id,
                                     scada::NodeObserver& events) const {
  node_events_[node_id].AddObserver(&events);
}

void AddressSpaceImpl::UnsubscribeNode(const scada::NodeId& node_id,
                                       scada::NodeObserver& events) const {
  auto i = node_events_.find(node_id);
  assert(i != node_events_.end());
  auto& e = i->second;
  e.RemoveObserver(&events);
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

void AddressSpaceImpl::AddStaticNode(std::unique_ptr<scada::Node> node) {
  AddNode(*node);
  static_nodes_.emplace(node->id(), std::move(node));
}
