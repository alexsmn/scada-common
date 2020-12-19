#pragma once

#include "address_space/address_space.h"
#include "base/observer_list.h"
#include "core/configuration_types.h"
#include "core/node_attributes.h"
#include "core/status.h"

#include <map>
#include <memory>

namespace scada {
class ReferenceType;
struct PropertyIds;
}  // namespace scada

class Logger;

class AddressSpaceImpl : public scada::AddressSpace {
 public:
  explicit AddressSpaceImpl(
      std::shared_ptr<Logger> logger,
      scada::AddressSpace* parent_address_space = nullptr);
  virtual ~AddressSpaceImpl();

  const Logger& logger() const { return *logger_; }

  typedef std::map<scada::NodeId, scada::Node*> NodeMap;
  const NodeMap& node_map() const { return node_map_; }

  // Add not-owned node.
  void AddNode(scada::Node& node);

  template <class T>
  T& AddStaticNode(std::unique_ptr<T> node);

  template <class T, class... Args>
  T& AddStaticNode(Args&&... args);

  // Deletes owned node.
  void RemoveNode(const scada::NodeId& id);

  void Clear();

  void ModifyNode(const scada::NodeId& id,
                  scada::NodeAttributes attributes,
                  scada::NodeProperties properties);

  void NotifyNodeAdded(const scada::Node& node) const;
  void NotifyNodeDeleted(const scada::Node& node) const;
  void NotifyNodeModified(const scada::Node& node,
                          const scada::PropertyIds& property_ids) const;

  void NotifyReference(const scada::ReferenceType& reference_type,
                       const scada::Node& source,
                       const scada::Node& target,
                       bool added) const;

  // scada::AddressSpace
  scada::Node* GetNode(const scada::NodeId& node_id) override;
  void Subscribe(scada::NodeObserver& events) const override;
  void Unsubscribe(scada::NodeObserver& events) const override;
  void SubscribeNode(const scada::NodeId& node_id,
                     scada::NodeObserver& events) const override;
  void UnsubscribeNode(const scada::NodeId& node_id,
                       scada::NodeObserver& events) const override;

 private:
  // Adds node owned by configuration itself.
  void AddStaticNodeHelper(std::unique_ptr<scada::Node> node);

  void NotifyNodeMoved(const scada::Node& node, const scada::Node* top) const;
  void NotifyNodeTitleChanged(const scada::Node& node) const;

  typedef base::ObserverList<scada::NodeObserver> NodeEvents;
  const NodeEvents* GetNodeEvents(const scada::NodeId& node_id) const;

  std::shared_ptr<Logger> logger_;

  scada::AddressSpace* parent_address_space_ = nullptr;

  NodeMap node_map_;

  std::map<scada::NodeId, std::unique_ptr<scada::Node>> static_nodes_;

  mutable NodeEvents observers_;
  mutable std::map<scada::NodeId, NodeEvents> node_events_;

  DISALLOW_COPY_AND_ASSIGN(AddressSpaceImpl);
};

template <class T>
inline T& AddressSpaceImpl::AddStaticNode(std::unique_ptr<T> node) {
  assert(node);
  auto* ptr = node.get();
  AddStaticNodeHelper(std::unique_ptr<scada::Node>{std::move(node)});
  return *ptr;
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
