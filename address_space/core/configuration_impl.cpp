#include "core/configuration_impl.h"

#include "core/attribute_ids.h"
#include "core/node_observer.h"
#include "core/node_utils.h"
#include "core/object.h"
#include "core/reference.h"
#include "core/standard_node_ids.h"
#include "core/status.h"
#include "core/type_definition.h"
#include "core/types.h"
#include "core/variable.h"

ConfigurationImpl::ConfigurationImpl(std::shared_ptr<Logger> logger)
    : logger_(std::move(logger)),
      standard_address_space_(std::make_unique<StandardAddressSpace>()),
      static_address_space_(
          std::make_unique<StaticAddressSpace>(*standard_address_space_)) {
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
  AddNode(standard_address_space_->ModellingRule_Mandatory);
  AddNode(standard_address_space_->FolderType);
  AddNode(standard_address_space_->BaseVariableType);
  AddNode(standard_address_space_->PropertyType);

  AddNode(static_address_space_->Creates);
  AddNode(static_address_space_->DeviceType);
  AddNode(static_address_space_->DeviceType.Disabled);
  AddNode(static_address_space_->DeviceType.Enabled);
  AddNode(static_address_space_->DeviceType.Online);
}

ConfigurationImpl::~ConfigurationImpl() {
  for (auto& p : node_map_)
    DeleteAllReferences(*p.second);
  node_map_.clear();
}

void ConfigurationImpl::ModifyNode(const scada::NodeId& id,
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

void ConfigurationImpl::AddNode(scada::Node& node) {
  assert(!node.id().is_null());
  assert(!GetNode(node.id()));

  auto& mapped_node = node_map_[node.id()];
  assert(!mapped_node);
  mapped_node = &node;
}

void AddNodeAndReference(ConfigurationImpl& address_space,
                         scada::Node& node,
                         const scada::NodeId& reference_type_id,
                         const scada::NodeId& parent_id) {
  auto* parent = address_space.GetNode(parent_id);
  assert(parent);

  AddNodeAndReference(address_space, std::move(node), reference_type_id,
                      *parent);
}

void AddNodeAndReference(ConfigurationImpl& address_space,
                         scada::Node& node,
                         const scada::NodeId& reference_type_id,
                         scada::Node& parent) {
  auto node_id = node.id();
  address_space.AddNode(node);
  AddReference(address_space, reference_type_id, parent, node);
}

void ConfigurationImpl::RemoveNode(const scada::NodeId& id) {
  NodeMap::iterator i = node_map_.find(id);
  assert(i != node_map_.end());

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

scada::Node* ConfigurationImpl::GetNode(const scada::NodeId& node_id) {
  auto i = node_map_.find(node_id);
  return i != node_map_.end() ? i->second : nullptr;
}

void ConfigurationImpl::Subscribe(scada::NodeObserver& events) const {
  observers_.AddObserver(&events);
}

void ConfigurationImpl::Unsubscribe(scada::NodeObserver& events) const {
  assert(observers_.HasObserver(&events));
  observers_.RemoveObserver(&events);
}

void ConfigurationImpl::SubscribeNode(const scada::NodeId& node_id,
                                      scada::NodeObserver& events) const {
  node_events_[node_id].AddObserver(&events);
}

void ConfigurationImpl::UnsubscribeNode(const scada::NodeId& node_id,
                                        scada::NodeObserver& events) const {
  auto i = node_events_.find(node_id);
  assert(i != node_events_.end());
  auto& e = i->second;
  e.RemoveObserver(&events);
}

void ConfigurationImpl::NotifyNodeAdded(const scada::Node& node) const {
  for (auto* parent = &node; parent; parent = GetParent(*parent)) {
    if (auto* events = GetNodeEvents(node.id())) {
      for (auto& o : *events)
        o.OnNodeCreated(node);
    }
  }

  for (auto& o : observers_)
    o.OnNodeCreated(node);
}

void ConfigurationImpl::NotifyNodeDeleted(const scada::Node& node) const {
  for (auto* parent = &node; parent; parent = GetParent(*parent)) {
    if (auto* events = GetNodeEvents(node.id())) {
      for (auto& o : *events)
        o.OnNodeDeleted(node);
    }
  }

  for (auto& o : observers_)
    o.OnNodeDeleted(node);
}

void ConfigurationImpl::NotifyNodeMoved(const scada::Node& node,
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

void ConfigurationImpl::NotifyNodeTitleChanged(const scada::Node& node) const {
  for (auto* parent = &node; parent; parent = GetParent(*parent)) {
    if (auto* events = GetNodeEvents(node.id())) {
      for (auto& o : *events)
        o.OnNodeTitleChanged(node);
    }
  }

  for (auto& o : observers_)
    o.OnNodeTitleChanged(node);
}

void ConfigurationImpl::NotifyNodeModified(
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

void ConfigurationImpl::NotifyReference(
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

const ConfigurationImpl::NodeEvents* ConfigurationImpl::GetNodeEvents(
    const scada::NodeId& node_id) const {
  auto i = node_events_.find(node_id);
  return i == node_events_.end() ? nullptr : &i->second;
}

void ConfigurationImpl::AddStaticNode(std::unique_ptr<scada::Node> node) {
  AddNode(*node);
  static_nodes_.emplace(node->id(), std::move(node));
}
