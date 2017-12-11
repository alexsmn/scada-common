#pragma once

#include <optional>
#include <vector>

#include "common/node_model.h"

class Logger;
class NodeModelImpl;
class NodeRefServiceImpl;

struct NodeModelImplReference {
  std::shared_ptr<const NodeModelImpl> reference_type;
  std::shared_ptr<const NodeModelImpl> target;
  bool forward;
};

class NodeModelImpl : public std::enable_shared_from_this<NodeModelImpl>,
                      public NodeModel {
 public:
  NodeModelImpl(NodeRefServiceImpl& service, scada::NodeId id, std::shared_ptr<const Logger> logger);

  virtual scada::Status GetStatus() const final;
  virtual bool IsFetched() const override { return fetched_; }
  virtual void Fetch(const NodeRef::FetchCallback& callback) const final;
  virtual scada::Variant GetAttribute(scada::AttributeId attribute_id) const final;
  virtual scada::Variant GetValue() const final;
  virtual NodeRef GetTypeDefinition() const final;
  virtual NodeRef GetSupertype() const final;
  virtual NodeRef GetDataType() const final;
  virtual NodeRef GetAggregate(const scada::QualifiedName& aggregate_name) const final;
  virtual std::vector<NodeRef> GetAggregates(const scada::NodeId& reference_type_id) const final;
  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id) const final;
  virtual std::vector<NodeRef> GetTargets(const scada::NodeId& reference_type_id) const final;
  virtual std::vector<NodeRef::Reference> GetReferences() const final;
  virtual NodeRef GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const final;
  virtual NodeRef GetParent() const final;
  virtual void Browse(const scada::BrowseDescription& description, const NodeRef::BrowseCallback& callback) const final;
  virtual void AddObserver(NodeRefObserver& observer) const final;
  virtual void RemoveObserver(NodeRefObserver& observer) const final;

 private:
  void OnReadComplete(scada::Status&& status, std::vector<scada::DataValue>&& data_values);
  void OnBrowseComplete(scada::Status&& status, std::vector<scada::BrowseResult>&& results);

  void SetAttribute(scada::AttributeId attribute_id, scada::Variant&& value);
  void AddReference(const NodeModelImplReference& reference);

  bool IsNodeFetched(std::vector<scada::NodeId>& fetched_node_ids) const;
  bool IsNodeFetchedHelper(std::vector<scada::NodeId>& fetched_node_ids) const;

  void SetError(const scada::Status& status);

  NodeRefServiceImpl& service_;
  const scada::NodeId id_;

  bool fetched_ = false;

  std::optional<scada::NodeClass> node_class_;
  scada::QualifiedName browse_name_;
  scada::LocalizedText display_name_;
  scada::Variant value_;
  scada::Status status_{scada::StatusCode::Good};

  std::vector<NodeModelImplReference> aggregates_;
  // Instance-only.
  std::shared_ptr<const NodeModelImpl> type_definition_;
  // Type-only.
  std::shared_ptr<const NodeModelImpl> supertype_;
  // Forward non-hierarchical.
  std::vector<NodeModelImplReference> references_;
  std::shared_ptr<const NodeModelImpl> data_type_;

  std::shared_ptr<const Logger> logger_;
  mutable unsigned pending_request_count_ = 0;
  std::vector<scada::NodeId> depended_ids_;
  mutable std::vector<NodeRef::FetchCallback> fetch_callbacks_;
  mutable bool passing_ = false;
  std::vector<NodeModelImplReference> pending_references_;

  friend class NodeRefServiceImpl;
};