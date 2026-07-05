#include "address_space/address_space_impl.h"
#include "base/check.h"

#include "address_space/address_space_util.h"
#include "address_space/object.h"
#include "address_space/property_ids.h"
#include "address_space/reference.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "scada/attribute_ids.h"
#include "scada/standard_node_ids.h"
#include "scada/status.h"

AddressSpaceImpl::AddressSpaceImpl(scada::AddressSpace* parent_address_space)
    : parent_address_space_{parent_address_space} {
  if (parent_address_space_)
    ConnectParentAddressSpace();
}

void AddressSpaceImpl::ConnectParentAddressSpace() {
  parent_connections_.push_back(parent_address_space_->SubscribeNodeCreated(
      [this](const scada::Node& node) { node_created_signal_(node); }));
  parent_connections_.push_back(parent_address_space_->SubscribeNodeDeleted(
      [this](const scada::Node& node) { node_deleted_signal_(node); }));
  parent_connections_.push_back(parent_address_space_->SubscribeNodeModified(
      [this](const scada::Node& node, const scada::PropertyIds& property_ids) {
        node_modified_signal_(node, property_ids);
      }));
  parent_connections_.push_back(parent_address_space_->SubscribeNodeMoved(
      [this](const scada::Node& node) { node_moved_signal_(node); }));
  parent_connections_.push_back(
      parent_address_space_->SubscribeNodeTitleChanged(
          [this](const scada::Node& node) {
            node_title_changed_signal_(node);
          }));
  parent_connections_.push_back(parent_address_space_->SubscribeReferenceAdded(
      [this](const scada::ReferenceType& reference_type,
             const scada::Node& source, const scada::Node& target) {
        reference_added_signal_(reference_type, source, target);
      }));
  parent_connections_.push_back(
      parent_address_space_->SubscribeReferenceDeleted(
          [this](const scada::ReferenceType& reference_type,
                 const scada::Node& source, const scada::Node& target) {
            reference_deleted_signal_(reference_type, source, target);
          }));
}

AddressSpaceImpl::~AddressSpaceImpl() {
  Clear();
}

void AddressSpaceImpl::Clear() {
  for (auto& p : node_map_)
    DeleteAllReferences(*this, *p.second);
  node_map_.clear();
}

bool AddressSpaceImpl::ModifyNode(const scada::NodeId& id,
                                  scada::NodeAttributes attributes,
                                  scada::NodeProperties properties) {
  auto* node = GetMutableNode(id);
  base::Check(node);

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
        // The value is data-dependent (may come from external
        // configuration); a failed update is ignored.
        (void)variable->SetValue(std::move(new_data_value));
      }

    } else {
      // A value for a non-variable node is external-data dependent; ignore.
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
    // Property values are data-dependent; a failed update is ignored.
    (void)scada::SetPropertyValue(*node, prop_decl_id, std::move(value));
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
  base::Check(!node.id().is_null());
  base::Check(!GetNode(node.id()));

  auto& mapped_node = node_map_[node.id()];
  base::Check(!mapped_node);
  mapped_node = &node;
}

void AddNodeAndReference(AddressSpaceImpl& address_space,
                         scada::Node& node,
                         const scada::NodeId& reference_type_id,
                         const scada::NodeId& parent_id) {
  auto* parent = address_space.GetMutableNode(parent_id);
  base::Check(parent);

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

void AddressSpaceImpl::DeleteNode(const scada::NodeId& id) {
  auto i = node_map_.find(id);
  if (i == node_map_.end())
    return;

  auto*& node = i->second;

  for (;;) {
    auto children = scada::GetChildren(*node);
    if (children.empty())
      break;
    auto& child = *children.front();
    DeleteNode(child.id());
  }

  DeleteAllReferences(*this, *node);

  NotifyNodeDeleted(*node);

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

boost::signals2::scoped_connection AddressSpaceImpl::SubscribeNodeCreated(
    const NodeCallback& callback) const {
  return node_created_signal_.connect(callback);
}

boost::signals2::scoped_connection AddressSpaceImpl::SubscribeNodeDeleted(
    const NodeCallback& callback) const {
  return node_deleted_signal_.connect(callback);
}

boost::signals2::scoped_connection AddressSpaceImpl::SubscribeNodeModified(
    const NodeModifiedCallback& callback) const {
  return node_modified_signal_.connect(callback);
}

boost::signals2::scoped_connection AddressSpaceImpl::SubscribeNodeMoved(
    const NodeCallback& callback) const {
  return node_moved_signal_.connect(callback);
}

boost::signals2::scoped_connection AddressSpaceImpl::SubscribeNodeTitleChanged(
    const NodeCallback& callback) const {
  return node_title_changed_signal_.connect(callback);
}

boost::signals2::scoped_connection AddressSpaceImpl::SubscribeReferenceAdded(
    const ReferenceCallback& callback) const {
  return reference_added_signal_.connect(callback);
}

boost::signals2::scoped_connection AddressSpaceImpl::SubscribeReferenceDeleted(
    const ReferenceCallback& callback) const {
  return reference_deleted_signal_.connect(callback);
}

void AddressSpaceImpl::NotifyNodeAdded(const scada::Node& node) const {
  node_created_signal_(node);
}

void AddressSpaceImpl::NotifyNodeDeleted(const scada::Node& node) const {
  node_deleted_signal_(node);
}

void AddressSpaceImpl::NotifyNodeMoved(const scada::Node& node,
                                       const scada::Node* top) const {
  node_moved_signal_(node);
}

void AddressSpaceImpl::NotifyNodeTitleChanged(const scada::Node& node) const {
  node_title_changed_signal_(node);
}

void AddressSpaceImpl::NotifyNodeModified(
    const scada::Node& node,
    const scada::PropertyIds& property_ids) const {
  node_modified_signal_(node, property_ids);
}

void AddressSpaceImpl::NotifyReference(
    const scada::ReferenceType& reference_type,
    const scada::Node& source,
    const scada::Node& target,
    bool added) const {
  if (added)
    reference_added_signal_(reference_type, source, target);
  else
    reference_deleted_signal_(reference_type, source, target);

  if (scada::IsSubtypeOf(reference_type, scada::id::HierarchicalReferences))
    NotifyNodeMoved(target, nullptr);
}

void AddressSpaceImpl::AddNode(std::unique_ptr<scada::Node> node) {
  AddNode(*node);
  static_nodes_.emplace(node->id(), std::move(node));
}

void AddressSpaceImpl::AddReference(const scada::ReferenceType& type,
                                    scada::Node& source,
                                    scada::Node& target) {
  source.AddReference(type, true, target);
  target.AddReference(type, false, source);

  NotifyReference(type, source, target, true);
}

void AddressSpaceImpl::DeleteReference(const scada::ReferenceType& type,
                                       scada::Node& source,
                                       scada::Node& target) {
  source.DeleteReference(type, true, target);
  target.DeleteReference(type, false, source);

  NotifyReference(type, source, target, false);
}