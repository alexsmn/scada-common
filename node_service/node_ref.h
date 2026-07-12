#pragma once

#include "base/awaitable.h"
#include "base/lifetime.h"
#include "node_service/node_events.h"
#include "node_service/node_fetch_status.h"
#include "scada/attribute_ids.h"
#include "scada/data_value.h"
#include "scada/node.h"
#include "scada/node_class.h"
#include "scada/node_id.h"
#include "scada/standard_node_ids.h"

#include <boost/signals2/connection.hpp>
#include <optional>
#include <vector>

class NodeService;

// Lightweight value cursor identifying one node within one NodeService.
//
// A NodeRef stores a node id plus a non-owning pointer to the service that
// owns the node. Every operation forwards to the service (see
// `node_service.h`); the ref itself carries no per-node state and does not pin
// the node resident. Identity is the stored node id, so comparison and hashing
// are O(1) with no attribute read.
//
// A NodeRef is only valid while its service is alive and must be used on the
// service's executor.
class NodeRef {
 public:
  NodeRef() noexcept = default;
  NodeRef(std::nullptr_t) noexcept {}
  // The cursor stores |service| non-owning, so it must not outlive it; the
  // lifetime annotation lets the compiler diagnose a NodeRef bound to a
  // shorter-lived service.
  NodeRef(scada::NodeId id, NodeService* service SCADA_LIFETIME_BOUND) noexcept
      : id_{std::move(id)}, service_{service} {}

  // Identity — no dispatch, no attribute read.
  const scada::NodeId& node_id() const noexcept SCADA_LIFETIME_BOUND {
    return id_;
  }
  NodeService* service() const noexcept { return service_; }

  explicit operator bool() const noexcept { return service_ != nullptr; }

  bool operator==(const NodeRef& other) const noexcept {
    return id_ == other.id_;
  }
  bool operator!=(const NodeRef& other) const noexcept {
    return !operator==(other);
  }
  bool operator<(const NodeRef& other) const noexcept {
    return id_ < other.id_;
  }

  scada::Status status() const;

  bool fetched() const;
  bool children_fetched() const;

  // On error: OnNodeFetched + OnNodeSemanticsChanged.
  Awaitable<NodeRef> Fetch(const NodeFetchStatus& requested_status =
                               NodeFetchStatus::NodeOnly) const;
  void StartFetch(const NodeFetchStatus& requested_status =
                      NodeFetchStatus::NodeOnly) const;

  scada::Variant attribute(scada::AttributeId attribute_id) const;

  std::optional<scada::NodeClass> node_class() const;
  scada::QualifiedName browse_name() const;
  scada::LocalizedText display_name() const;
  scada::Variant value() const;
  NodeRef data_type() const;

  NodeRef type_definition() const;
  NodeRef supertype() const;
  NodeRef parent() const;

  NodeRef target(const scada::NodeId& reference_type_id) const;
  NodeRef inverse_target(const scada::NodeId& reference_type_id) const;
  bool has_target(const scada::NodeId& reference_type_id,
                  bool forward,
                  const scada::NodeId& node_id) const;
  std::vector<NodeRef> targets(
      const scada::NodeId& reference_type_id = scada::id::References) const;
  std::vector<NodeRef> inverse_targets(
      const scada::NodeId& reference_type_id = scada::id::References) const;

  struct Reference;
  using References = std::vector<Reference>;

  // Non-hierarchical references.
  Reference reference(const scada::NodeId& reference_type_id) const;
  Reference inverse_reference(const scada::NodeId& reference_type_id) const;
  Reference reference(const scada::NodeId& reference_type_id,
                      bool forward,
                      const scada::NodeId& node_id) const;
  std::vector<Reference> references(
      const scada::NodeId& reference_type_id = scada::id::References) const;
  std::vector<Reference> inverse_references(
      const scada::NodeId& reference_type_id = scada::id::References) const;

  NodeRef operator[](const scada::QualifiedName& child_name) const;
  NodeRef operator[](const scada::NodeId& aggregate_declaration_id) const;

  // Subscriptions forward to the service; a null ref returns an empty
  // connection. An active subscription keeps the node resident.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeModelChanged(
      const ModelChangedCallback& callback) const;
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeSemanticChanged(
      const NodeSemanticChangedCallback& callback) const;
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeFetched(
      const NodeFetchedCallback& callback) const;
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeStateChanged(
      const NodeStateChangedCallback& callback) const;

  scada::node scada_node() const;

 private:
  scada::NodeId id_;
  NodeService* service_ = nullptr;
};

struct NodeRef::Reference {
  NodeRef reference_type;
  NodeRef target;
  bool forward;
};

namespace std {

template <>
struct hash<NodeRef> {
  std::size_t operator()(const NodeRef& node) const noexcept {
    return hash<scada::NodeId>{}(node.node_id());
  }
};

}  // namespace std
