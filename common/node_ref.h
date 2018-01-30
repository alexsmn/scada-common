#pragma once

#include "core/attribute_ids.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/status.h"
#include "core/variant.h"

#include <functional>
#include <memory>
#include <optional>

class NodeModel;
class NodeRefObserver;

class NodeRef {
 public:
  NodeRef() {}
  NodeRef(std::nullptr_t) {}
  template <class T>
  NodeRef(std::shared_ptr<T> model) : model_{std::move(model)} {}

  scada::Status status() const;

  bool fetched() const;
  using FetchCallback = std::function<void(const NodeRef& node)>;
  void Fetch(const FetchCallback& callback) const;

  scada::Variant attribute(scada::AttributeId attribute_id) const;

  scada::NodeId id() const;
  std::optional<scada::NodeClass> node_class() const;
  scada::QualifiedName browse_name() const;
  scada::LocalizedText display_name() const;
  scada::Variant value() const;
  NodeRef type_definition() const;
  NodeRef supertype() const;
  NodeRef data_type() const;

  NodeRef parent() const;

  // Includes components and properies.
  std::vector<NodeRef> aggregates() const;
  std::vector<NodeRef> components() const;
  std::vector<NodeRef> properties() const;

  NodeRef target(const scada::NodeId& reference_type_id) const;
  std::vector<NodeRef> targets(const scada::NodeId& reference_type_id) const;

  struct Reference;
  using References = std::vector<Reference>;

  // Non-hierarchical references.
  std::vector<Reference> references() const;

  NodeRef GetAggregateDeclaration(
      const scada::NodeId& aggregate_declaration_id) const;

  void Subscribe(NodeRefObserver& observer);
  void Unsubscribe(NodeRefObserver& observer);

  NodeRef operator[](const scada::QualifiedName& aggregate_name) const;
  NodeRef operator[](const scada::NodeId& aggregate_declaration_id) const;

  explicit operator bool() const { return model_ != nullptr; }

  bool operator==(const NodeRef& other) const { return id() == other.id(); }
  bool operator!=(const NodeRef& other) const { return !operator==(other); }

  bool operator<(const NodeRef& other) const { return id() < other.id(); }

 private:
  std::shared_ptr<const NodeModel> model_;
};

struct NodeRef::Reference {
  NodeRef reference_type;
  NodeRef target;
  bool forward;
};
