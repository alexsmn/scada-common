#pragma once

#include "address_space/mutable_address_space.h"
#include "base/check.h"
#include "scada/status.h"

#include <boost/signals2/signal.hpp>
#include <map>
#include <vector>

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
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeCreated(
      const NodeCallback& callback) const override;
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeDeleted(
      const NodeCallback& callback) const override;
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeModified(
      const NodeModifiedCallback& callback) const override;
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeMoved(
      const NodeCallback& callback) const override;
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeTitleChanged(
      const NodeCallback& callback) const override;
  [[nodiscard]] boost::signals2::scoped_connection SubscribeReferenceAdded(
      const ReferenceCallback& callback) const override;
  [[nodiscard]] boost::signals2::scoped_connection SubscribeReferenceDeleted(
      const ReferenceCallback& callback) const override;

 private:
  void NotifyNodeMoved(const scada::Node& node, const scada::Node* top) const;
  void NotifyNodeTitleChanged(const scada::Node& node) const;

  // Forwards all change notifications of |parent_address_space_| through this
  // instance's own signals, so subscribers observe the parent chain with a
  // single subscription.
  void ConnectParentAddressSpace();

  scada::AddressSpace* parent_address_space_ = nullptr;

  NodeMap node_map_;

  std::map<scada::NodeId, std::unique_ptr<scada::Node>> static_nodes_;

  mutable boost::signals2::signal<void(const scada::Node&)>
      node_created_signal_;
  mutable boost::signals2::signal<void(const scada::Node&)>
      node_deleted_signal_;
  mutable boost::signals2::signal<void(const scada::Node&,
                                       const scada::PropertyIds&)>
      node_modified_signal_;
  mutable boost::signals2::signal<void(const scada::Node&)> node_moved_signal_;
  mutable boost::signals2::signal<void(const scada::Node&)>
      node_title_changed_signal_;
  mutable boost::signals2::signal<
      void(const scada::ReferenceType&, const scada::Node&, const scada::Node&)>
      reference_added_signal_;
  mutable boost::signals2::signal<
      void(const scada::ReferenceType&, const scada::Node&, const scada::Node&)>
      reference_deleted_signal_;

  std::vector<boost::signals2::scoped_connection> parent_connections_;
};

template <class T>
inline T& AddressSpaceImpl::AddStaticNode(std::unique_ptr<T> node) {
  base::Check(node);
  auto& ref = *node;
  AddNode(std::move(node));
  // cppcheck-suppress returnReference
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
