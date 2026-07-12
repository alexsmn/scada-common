#pragma once

#include "base/lifetime.h"
#include "common/node_state.h"
#include "node_service/base_node_model.h"

#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace scada {
struct ModelChangeEvent;
}

namespace v3 {

class NodeModelImpl;
class NodeServiceImpl;
struct NodeModelRegistry;

struct RemoteReference {
  NodeModelImpl* reference_type = nullptr;
  NodeModelImpl* target = nullptr;
  bool forward = true;
};

using ReferenceMap =
    std::map<scada::NodeId /*reference_type_id*/,
             std::map<scada::NodeId /*target_id*/,
                      scada::NodeId /*child_reference_type_id*/>>;

// Node model for v3's direct NodeState cache.
//
// It follows the v2 model shape but relies on the service's injected
// NodeFetcher for loading. It tracks pending model/semantic notifications
// locally instead of using v2's PendingEvents helper.
class NodeModelImpl final : public BaseNodeModel,
                            public std::enable_shared_from_this<NodeModelImpl> {
 public:
  NodeModelImpl(NodeServiceImpl& service,
                std::shared_ptr<NodeModelRegistry> registry,
                scada::NodeId node_id);
  ~NodeModelImpl() override;

  // Identifier of the node this model represents.
  const scada::NodeId& node_id() const SCADA_LIFETIME_BOUND { return node_id_; }

  void OnModelChanged(const scada::ModelChangeEvent& event);

  void OnFetched(const scada::NodeState& node_state);
  void OnFetchCompleted();
  void OnFetchError(scada::Status&& status);
  void OnChildrenFetched(scada::ReferenceDescriptions&& references);

  // NodeModel
  virtual scada::Variant GetAttribute(
      scada::AttributeId attribute_id) const override;
  virtual NodeRef GetDataType() const override;
  virtual NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const override;
  virtual NodeRef GetChild(
      const scada::QualifiedName& child_name) const override;
  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id,
                            bool forward) const override;
  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  virtual NodeRef::Reference GetReference(
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& node_id) const override;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  virtual scada::node GetScadaNode() const override;

 protected:
  // BaseNodeModel
  virtual void OnFetchRequested(
      const NodeFetchStatus& requested_status) override;

 private:
  NodeRef GetAggregateDeclaration(
      const scada::NodeId& aggregate_declaration_id) const;

  void SetError(const scada::Status& status);

  void NotifyModelChanged();
  void NotifySemanticChanged();

  // Publishes a fresh immutable snapshot built from the working state and
  // notifies observers with it; deferred while a fetch batch holds the
  // callback lock.
  void NotifyStateChanged();

  NodeServiceImpl& service_;
  // Shared with the service so the destructor can unregister safely even if
  // the last NodeRef outlives the service.
  const std::shared_ptr<NodeModelRegistry> registry_;
  const scada::NodeId node_id_;

  // Working state, mutated in place by fetch results and remote updates.
  // Immutable snapshots of it are published to observers via
  // NotifyStateChanged.
  scada::NodeState node_state_;
  scada::ReferenceDescriptions child_references_;

  // Pins for the fetched children (and their reference types): the parent
  // keeps its fetched subtree resident, so dropping the last NodeRef to a
  // subtree root releases the whole subtree.
  std::vector<NodeRef> child_models_;

  std::shared_ptr<bool> reference_request_;

  bool pending_model_changed_ = false;
  bool pending_semantic_changed_ = false;
  bool pending_state_changed_ = false;

  friend class NodeServiceImpl;
};

}  // namespace v3
