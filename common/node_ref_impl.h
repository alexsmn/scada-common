#pragma once

#include <vector>

#include "common/node_ref.h"

class NodeRefServiceImpl;

struct NodeRefImplReference {
  std::shared_ptr<NodeRefImpl> reference_type;
  std::shared_ptr<NodeRefImpl> target;
  bool forward;
};

class NodeRefImpl : public std::enable_shared_from_this<NodeRefImpl> {
 public:
  NodeRefImpl(NodeRefServiceImpl& service, scada::NodeId id) : service_{service}, id_{std::move(id)} {}
  virtual ~NodeRefImpl() {}

  scada::Status GetStatus() const;

  bool IsFetched() const { return fetched_; }
  void Fetch(const NodeRef::FetchCallback& callback);

  scada::DataValue GetAttribute(scada::AttributeId attribute_id) const;

  NodeRef GetTypeDefinition() const;
  NodeRef GetSupertype() const;
  NodeRef GetDataType() const;

  NodeRef GetAggregate(base::StringPiece aggregate_name) const;
  std::vector<NodeRef> GetAggregates(const scada::NodeId& reference_type_id) const;

  NodeRef GetTarget(const scada::NodeId& reference_type_id) const;
  std::vector<NodeRef> GetTargets(const scada::NodeId& reference_type_id) const;

  std::vector<NodeRef::Reference> GetReferences() const;

  NodeRef GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const;

  void Browse(const scada::BrowseDescription& description, const NodeRef::BrowseCallback& callback);

  void AddObserver(NodeRefObserver& observer);
  void RemoveObserver(NodeRefObserver& observer);

 private:
  NodeRefServiceImpl& service_;
  const scada::NodeId id_;

  bool fetched_;

  scada::NodeClass node_class_;
  std::string browse_name_;
  base::string16 display_name_;
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

  friend class NodeRefServiceImpl;
};
