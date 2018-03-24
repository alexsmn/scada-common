#pragma once

#include "common/node_model.h"

#include <optional>
#include <vector>

namespace scada {
class DataValue;
struct BrowseResult;
}  // namespace scada

class Logger;
class RemoteNodeModel;
class RemoteNodeService;

struct NodeModelImplReference {
  std::shared_ptr<const RemoteNodeModel> reference_type;
  std::shared_ptr<const RemoteNodeModel> target;
  bool forward;
};

class RemoteNodeModel final
    : public std::enable_shared_from_this<RemoteNodeModel>,
      public NodeModel {
 public:
  RemoteNodeModel(RemoteNodeService& service,
                  scada::NodeId id,
                  std::shared_ptr<const Logger> logger);

  // NodeModel
  virtual scada::Status GetStatus() const override;
  virtual NodeFetchStatus GetFetchStatus() const override;
  virtual void Fetch(const NodeFetchStatus& requested_status,
                     const FetchCallback& callback) const override;
  virtual scada::Variant GetAttribute(
      scada::AttributeId attribute_id) const override;
  virtual NodeRef GetDataType() const override;
  virtual NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const override;
  virtual NodeRef GetAggregate(
      const scada::QualifiedName& aggregate_name) const override;
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
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;

 private:
  void OnReadComplete(scada::Status&& status,
                      std::vector<scada::DataValue>&& data_values);
  void OnBrowseComplete(scada::Status&& status,
                        std::vector<scada::BrowseResult>&& results);

  void SetAttribute(scada::AttributeId attribute_id, scada::Variant&& value);
  void AddReference(const NodeModelImplReference& reference);

  bool IsNodeFetched(std::vector<scada::NodeId>& fetched_node_ids) const;
  bool IsNodeFetchedHelper(std::vector<scada::NodeId>& fetched_node_ids) const;

  NodeRef GetAggregateDeclaration(
      const scada::NodeId& aggregate_declaration_id) const;

  void SetError(const scada::Status& status);

  RemoteNodeService& service_;
  const scada::NodeId id_;

  NodeFetchStatus fetch_status_{};

  std::optional<scada::NodeClass> node_class_;
  scada::QualifiedName browse_name_;
  scada::LocalizedText display_name_;
  scada::Variant value_;
  scada::Status status_{scada::StatusCode::Good};

  std::vector<NodeModelImplReference> references_;

  std::shared_ptr<const RemoteNodeModel> type_definition_;
  std::shared_ptr<const RemoteNodeModel> supertype_;
  std::shared_ptr<const RemoteNodeModel> data_type_;

  std::shared_ptr<const Logger> logger_;
  mutable unsigned pending_request_count_ = 0;
  std::vector<scada::NodeId> depended_ids_;
  mutable std::vector<FetchCallback> fetch_callbacks_;
  mutable bool passing_ = false;
  std::vector<NodeModelImplReference> pending_references_;

  friend class RemoteNodeService;
};
