#pragma once

#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "node_service/base_node_model.h"

namespace scada {
struct ModelChangeEvent;
}  // namespace scada

namespace v1 {

class NodeModelImpl;

// Bridge from an individual v1 node model back to the AddressSpace-backed
// service that owns the local graph and fetch scheduler.
class NodeModelDelegate {
 public:
  virtual ~NodeModelDelegate() {}

  // If |node| is nullptr, empty node model is returned.
  virtual NodeRef GetRemoteNode(const scada::Node* node) = 0;

  virtual void OnNodeModelDeleted(const scada::NodeId& node_id) = 0;

  virtual void OnNodeModelFetchRequested(
      const scada::NodeId& node_id,
      const NodeFetchStatus& requested_status) = 0;
};

struct NodeModelImplContext {
  NodeModelDelegate& delegate_;
  const scada::NodeId node_id_;
  scada::node scada_node_;
};

// Node model for v1's AddressSpace mirror.
//
// The model reads attributes and references from a local scada::node and asks
// NodeServiceImpl to fetch missing remote data. v2/v3 models store fetched
// NodeState directly instead of reading through AddressSpace nodes.
class NodeModelImpl final : private NodeModelImplContext,
                            public BaseNodeModel,
                            public std::enable_shared_from_this<NodeModelImpl> {
 public:
  explicit NodeModelImpl(NodeModelImplContext&& context);
  ~NodeModelImpl();

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnNodeSemanticChanged();

  void SetFetchStatus(const scada::Node* node,
                      const scada::Status& status,
                      const NodeFetchStatus& fetch_status);
  void NotifyFetchStatus();

  // NodeModel
  virtual scada::Variant GetAttribute(
      scada::AttributeId attribute_id) const override;
  virtual NodeRef::Reference GetReference(
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& node_id) const override;
  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id,
                            bool forward) const override;
  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  virtual NodeRef GetDataType() const override;
  virtual NodeRef GetAggregate(
      const scada::NodeId& declaration_id) const override;
  virtual NodeRef GetChild(
      const scada::QualifiedName& child_name) const override;
  virtual void OnNodeDeleted() override;
  virtual scada::node GetScadaNode() const override;

 protected:
  // BaseNodeModel
  virtual void OnFetchRequested(
      const NodeFetchStatus& requested_status) override;

  const scada::Node* node_ = nullptr;
};

}  // namespace v1
