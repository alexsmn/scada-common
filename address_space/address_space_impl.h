#pragma once

#include "address_space/mutable_address_space.h"
#include "base/observer_list.h"
#include "core/status.h"

#include <map>

namespace scada {
struct PropertyIds;
}  // namespace scada

class AddressSpaceImpl : public MutableAddressSpace {
 public:
  explicit AddressSpaceImpl(
      scada::AddressSpace* parent_address_space = nullptr);
  ~AddressSpaceImpl();

  AddressSpaceImpl(const AddressSpaceImpl&) = delete;
  AddressSpaceImpl& operator=(const AddressSpaceImpl&) = delete;

  typedef std::map<scada::NodeId, scada::Node*> NodeMap;
  const NodeMap& node_map() const { return node_map_; }

  // Add not-owned node.
  void AddNode(scada::Node& node);

  virtual void AddNode(std::unique_ptr<scada::Node> node) override;

  template <class T>
  T& AddStaticNode(std::unique_ptr<T> node);

  template <class T, class... Args>
  T& AddStaticNode(Args&&... args);

  // Deletes owned node.
  virtual void DeleteNode(const scada::NodeId& id) override;

  void Clear();

  // Returns false if nothing was changed.
  virtual bool ModifyNode(const scada::NodeId& id,
                          scada::NodeAttributes attributes,
                          scada::NodeProperties properties) override;

  virtual void AddReference(const scada::ReferenceType& type,
                            scada::Node& source,
                            scada::Node& target) override;

  virtual void DeleteReference(const scada::ReferenceType& type,
                               scada::Node& source,
                               scada::Node& target) override;

  void NotifyNodeAdded(const scada::Node& node) const;
  void NotifyNodeDeleted(const scada::Node& node) const;
  void NotifyNodeModified(const scada::Node& node,
                          const scada::PropertyIds& property_ids) const;

  void NotifyReference(const scada::ReferenceType& reference_type,
                       const scada::Node& source,
                       const scada::Node& target,
                       bool added) const;

  // scada::AddressSpace
  scada::Node* GetMutableNode(const scada::NodeId& node_id) override;
  const scada::Node* GetNode(const scada::NodeId& node_id) const override;
  void Subscribe(scada::NodeObserver& events) const override;
  void Unsubscribe(scada::NodeObserver& events) const override;
  void SubscribeNode(const scada::NodeId& node_id,
                     scada::NodeObserver& events) const override;
  void UnsubscribeNode(const scada::NodeId& node_id,
                       scada::NodeObserver& events) const override;

 private:
  void NotifyNodeMoved(const scada::Node& node, const scada::Node* top) const;
  void NotifyNodeTitleChanged(const scada::Node& node) const;

  typedef base::ObserverList<scada::NodeObserver> NodeEvents;
  const NodeEvents* GetNodeEvents(const scada::NodeId& node_id) const;

  scada::AddressSpace* parent_address_space_ = nullptr;

  NodeMap node_map_;

  std::map<scada::NodeId, std::unique_ptr<scada::Node>> static_nodes_;

  mutable NodeEvents observers_;
  mutable std::map<scada::NodeId, NodeEvents> node_events_;
};

template <class T>
inline T& AddressSpaceImpl::AddStaticNode(std::unique_ptr<T> node) {
  assert(node);
  auto& ref = *node;
  AddNode(std::move(node));
  return ref;
}

template <class T, class... Args>
inline T& AddressSpaceImpl::AddStaticNode(Args&&... args) {
  return AddStaticNode(std::make_unique<T>(std::forward<Args>(args)...));
}

void AddNodeAndReference(AddressSpaceImpl& address_space,
                         scada::Node& node,
                         const scada::NodeId& reference_type_id,
                         const scada::NodeId& parent_id);
void AddNodeAndReference(AddressSpaceImpl& address_space,
                         scada::Node& node,
                         const scada::NodeId& reference_type_id,
                         scada::Node& parent);
