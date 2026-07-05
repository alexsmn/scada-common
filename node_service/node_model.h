#pragma once

#include "base/awaitable.h"
#include "node_service/node_events.h"
#include "node_service/node_ref.h"
#include "scada/client.h"

#include <boost/signals2/connection.hpp>
#include <vector>

class NodeModel {
 public:
  virtual ~NodeModel() {}

  virtual scada::Status GetStatus() const = 0;

  virtual NodeFetchStatus GetFetchStatus() const = 0;

  virtual Awaitable<void> Fetch(
      const NodeFetchStatus& requested_status) const = 0;
  virtual void StartFetch(const NodeFetchStatus& requested_status) const = 0;

  virtual scada::Variant GetAttribute(
      scada::AttributeId attribute_id) const = 0;

  virtual NodeRef GetDataType() const = 0;

  virtual NodeRef::Reference GetReference(
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& node_id) const = 0;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const = 0;

  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id,
                            bool forward) const = 0;
  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& reference_type_id,
      bool forward) const = 0;

  virtual NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const = 0;
  virtual NodeRef GetChild(const scada::QualifiedName& child_name) const = 0;

  // Notifies about model changes affecting this node.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeModelChanged(const ModelChangedCallback& callback) const = 0;
  // Notifies about displayed attribute changes and fetch errors of this node.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const NodeSemanticChangedCallback& callback) const = 0;
  // Notifies when a fetch of this node completed.
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const NodeFetchedCallback& callback) const = 0;
  // Notifies after this node model swapped in a new state snapshot.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(const NodeStateChangedCallback& callback) const = 0;

  virtual scada::node GetScadaNode() const = 0;
};
