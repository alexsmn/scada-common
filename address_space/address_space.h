#pragma once

#include <boost/signals2/connection.hpp>
#include <functional>

namespace scada {

class Node;
class NodeId;
class ReferenceType;
struct PropertyIds;

// Read-only view over a node graph with change notifications.
class AddressSpace {
 public:
  using NodeCallback = std::function<void(const Node& node)>;
  using NodeModifiedCallback =
      std::function<void(const Node& node, const PropertyIds& property_ids)>;
  using ReferenceCallback =
      std::function<void(const ReferenceType& reference_type,
                         const Node& source,
                         const Node& target)>;

  virtual ~AddressSpace() {}

  virtual Node* GetMutableNode(const NodeId& node_id) = 0;
  virtual const Node* GetNode(const NodeId& node_id) const = 0;

  // Notifies after a node has been added to the address space.
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeCreated(
      const NodeCallback& callback) const = 0;
  // Notifies right before a node is removed; the node is still alive for the
  // duration of the callback.
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeDeleted(
      const NodeCallback& callback) const = 0;
  // Notifies after node attributes or properties changed.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeModified(const NodeModifiedCallback& callback) const = 0;
  // Notifies after a node has been re-parented via a hierarchical reference.
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeMoved(
      const NodeCallback& callback) const = 0;
  // Notifies after a node display title changed.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeTitleChanged(const NodeCallback& callback) const = 0;
  // Notifies after a reference between two nodes has been added.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeReferenceAdded(const ReferenceCallback& callback) const = 0;
  // Notifies after a reference between two nodes has been deleted.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeReferenceDeleted(const ReferenceCallback& callback) const = 0;
};

}  // namespace scada
