#pragma once

#include "base/boost_log.h"
#include "base/observer_list.h"
#include "common/node_state.h"
#include "node_service/base_node_model.h"

#include <map>
#include <vector>

namespace scada {
struct ModelChangeEvent;
}

namespace v2 {

class NodeModelImpl;
class NodeServiceImpl;
class PendingEvents;

struct RemoteReference {
  NodeModelImpl* reference_type = nullptr;
  NodeModelImpl* target = nullptr;
  bool forward = true;
};

using ReferenceMap =
    std::map<scada::NodeId /*reference_type_id*/,
             std::map<scada::NodeId /*target_id*/,
                      scada::NodeId /*child_reference_type_id*/>>;

class NodeModelImpl final : public BaseNodeModel,
                            public std::enable_shared_from_this<NodeModelImpl> {
 public:
  NodeModelImpl(NodeServiceImpl& service, scada::NodeId node_id);

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
  virtual void OnFetchStatusChanged() override;

 private:
  NodeRef GetAggregateDeclaration(
      const scada::NodeId& aggregate_declaration_id) const;

  void SetError(const scada::Status& status);

  void NotifyModelChanged();
  void NotifySemanticChanged();

  NodeServiceImpl& service_;
  const scada::NodeId node_id_;

  scada::NodeState node_state_;
  scada::ReferenceDescriptions child_references_;
  scada::ReferenceDescriptions inverse_references_;

  std::shared_ptr<bool> reference_request_;

  friend class NodeServiceImpl;
  friend class PendingEvents;
};

}  // namespace v2
