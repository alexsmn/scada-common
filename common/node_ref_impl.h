#pragma once

#include <optional>
#include <vector>

#include "common/node_ref.h"

class Logger;
class NodeRefServiceImpl;

struct NodeRefImplReference {
  std::shared_ptr<NodeRefImpl> reference_type;
  std::shared_ptr<NodeRefImpl> target;
  bool forward;
};

class NodeRefImpl : public std::enable_shared_from_this<NodeRefImpl> {
 public:
  NodeRefImpl(NodeRefServiceImpl& service, scada::NodeId id, std::shared_ptr<const Logger> logger);
  virtual ~NodeRefImpl() {}

  scada::Status GetStatus() const;

  bool IsFetched() const { return fetched_; }
  void Fetch(const NodeRef::FetchCallback& callback);

  scada::Variant GetAttribute(scada::AttributeId attribute_id) const;
  scada::DataValue GetValue() const;

  NodeRef GetTypeDefinition() const;
  NodeRef GetSupertype() const;
  NodeRef GetDataType() const;

  NodeRef GetAggregate(const scada::QualifiedName& aggregate_name) const;
  std::vector<NodeRef> GetAggregates(const scada::NodeId& reference_type_id) const;

  NodeRef GetTarget(const scada::NodeId& reference_type_id) const;
  std::vector<NodeRef> GetTargets(const scada::NodeId& reference_type_id) const;

  std::vector<NodeRef::Reference> GetReferences() const;

  NodeRef GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const;

  void Browse(const scada::BrowseDescription& description, const NodeRef::BrowseCallback& callback);

  void AddObserver(NodeRefObserver& observer);
  void RemoveObserver(NodeRefObserver& observer);

 private:
  void OnReadComplete(const scada::Status& status, std::vector<scada::DataValue> values);
  void OnBrowseComplete(const scada::Status& status, std::vector<scada::BrowseResult> results);

  void SetAttribute(scada::AttributeId attribute_id, scada::DataValue data_value);
  void AddReference(const NodeRefImplReference& reference);

  bool IsNodeFetched(std::vector<scada::NodeId>& fetched_node_ids);
  bool IsNodeFetchedHelper(std::vector<scada::NodeId>& fetched_node_ids);

  void SetError(const scada::Status& status);

  NodeRefServiceImpl& service_;
  const scada::NodeId id_;

  bool fetched_ = false;

  std::optional<scada::NodeClass> node_class_;
  scada::QualifiedName browse_name_;
  scada::LocalizedText display_name_;
  scada::DataValue data_value_;
  scada::Status status_{scada::StatusCode::Good};

  std::vector<NodeRefImplReference> aggregates_;
  // Instance-only.
  std::shared_ptr<NodeRefImpl> type_definition_;
  // Type-only.
  std::shared_ptr<NodeRefImpl> supertype_;
  // Forward non-hierarchical.
  std::vector<NodeRefImplReference> references_;
  std::shared_ptr<NodeRefImpl> data_type_;

  std::shared_ptr<const Logger> logger_;
  unsigned pending_request_count_ = 0;
  std::vector<scada::NodeId> depended_ids_;
  std::vector<NodeRef::FetchCallback> fetch_callbacks_;
  bool passing_ = false;

  friend class NodeRefServiceImpl;
};
