#pragma once

#include "base/observer_list.h"
#include "common/base_node_model.h"
#include "common/node_state.h"

#include <map>
#include <optional>
#include <vector>

namespace scada {
struct ModelChangeEvent;
}

class RemoteNodeModel;
class RemoteNodeService;

struct NodeModelImplReference {
  std::shared_ptr<RemoteNodeModel> reference_type;
  std::shared_ptr<RemoteNodeModel> target;
  bool forward;
};

using ReferenceMap =
    std::map<scada::NodeId /*reference_type_id*/,
             std::map<scada::NodeId /*target_id*/,
                      scada::NodeId /*child_reference_type_id*/>>;

class RemoteNodeModel final
    : public BaseNodeModel,
      public std::enable_shared_from_this<RemoteNodeModel> {
 public:
  RemoteNodeModel(RemoteNodeService& service, scada::NodeId node_id);

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnNodeSemanticChanged();

  void OnNodeFetched(scada::NodeState&& node_state);
  void OnChildrenFetched(const ReferenceMap& references);

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
      bool forward) const override;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  virtual std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(
      scada::AttributeId attribute_id) const override;
  virtual void Write(const scada::NodeId& node_id,
                     double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const scada::StatusCallback& callback) const override;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::StatusCallback& callback) const override;

 protected:
  // BaseNodeModel
  virtual void OnFetchRequested(
      const NodeFetchStatus& requested_status) override;

 private:
  void AddReference(const NodeModelImplReference& reference);

  NodeRef GetAggregateDeclaration(
      const scada::NodeId& aggregate_declaration_id) const;

  void SetError(const scada::Status& status);

  RemoteNodeService& service_;

  std::optional<scada::NodeClass> node_class_;
  scada::NodeAttributes attributes_;
  scada::Status status_{scada::StatusCode::Good};

  std::vector<NodeModelImplReference> references_;
  std::vector<NodeModelImplReference> child_references_;

  NodeModelImplReference parent_reference_;

  std::shared_ptr<RemoteNodeModel> type_definition_;
  std::shared_ptr<RemoteNodeModel> supertype_;
  std::shared_ptr<RemoteNodeModel> data_type_;

  friend class RemoteNodeService;
};
