#pragma once

#include <vector>

#include "common/node_ref.h"

class NodeModel {
 public:
  virtual ~NodeModel() {}

  virtual scada::Status GetStatus() const = 0;

  virtual bool IsFetched() const = 0;
  virtual void Fetch(const NodeRef::FetchCallback& callback) const = 0;

  virtual scada::Variant GetAttribute(scada::AttributeId attribute_id) const = 0;
  virtual scada::DataValue GetValue() const = 0;

  virtual NodeRef GetTypeDefinition() const = 0;
  virtual NodeRef GetSupertype() const = 0;
  virtual NodeRef GetDataType() const = 0;

  virtual NodeRef GetAggregate(const scada::QualifiedName& aggregate_name) const = 0;
  virtual std::vector<NodeRef> GetAggregates(const scada::NodeId& reference_type_id) const = 0;

  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id) const = 0;
  virtual std::vector<NodeRef> GetTargets(const scada::NodeId& reference_type_id) const = 0;

  virtual std::vector<NodeRef::Reference> GetReferences() const = 0;

  virtual NodeRef GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const = 0;

  virtual void Browse(const scada::BrowseDescription& description, const NodeRef::BrowseCallback& callback) const = 0;

  virtual void AddObserver(NodeRefObserver& observer) const = 0;
  virtual void RemoveObserver(NodeRefObserver& observer) const = 0;
};