#pragma once

#include <functional>
#include <memory>

#include "base/optional.h"
#include "core/attribute_ids.h"
#include "core/data_value.h"
#include "core/node_id.h"
#include "core/node_types.h"
#include "core/status.h"
#include "core/view_service.h"

class NodeRefImpl;
class NodeRefObserver;

class NodeRef {
 public:
  NodeRef() {}
  NodeRef(std::nullptr_t) {}
  explicit NodeRef(std::shared_ptr<NodeRefImpl> impl);

  scada::NodeId id() const;

  scada::Status status() const;

  bool fetched() const;

  using FetchCallback = std::function<void(NodeRef node)>;
  void Fetch(const FetchCallback& callback);

  base::Optional<scada::NodeClass> node_class() const;
  scada::QualifiedName browse_name() const;
  scada::LocalizedText display_name() const;
  NodeRef type_definition() const;
  NodeRef supertype() const;
  NodeRef data_type() const;

  // Includes components and properies.
  std::vector<NodeRef> aggregates() const;
  std::vector<NodeRef> components() const;
  std::vector<NodeRef> properties() const;

  struct Reference;
  using References = std::vector<Reference>;

  // Non-hierarchical references.
  std::vector<Reference> references() const;

  scada::DataValue data_value() const;
  scada::Variant value() const;

  explicit operator bool() const { return !is_null(); }

  bool operator==(const NodeRef& other) const;
  bool operator!=(const NodeRef& other) const;

  NodeRef& operator=(std::nullptr_t);
  bool operator==(std::nullptr_t) const { return is_null(); }
  bool operator!=(std::nullptr_t) const { return !is_null(); }

  NodeRef operator[](const scada::QualifiedName& aggregate_name) const;
  NodeRef operator[](const scada::NodeId& aggregate_declaration_id) const;

  NodeRef target(const scada::NodeId& reference_type_id) const;
  std::vector<NodeRef> targets(const scada::NodeId& reference_type_id) const;

  NodeRef GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const;

  using BrowseCallback = std::function<void(const scada::Status& status, scada::ReferenceDescriptions references)>;
  void Browse(const scada::BrowseDescription& description, const BrowseCallback& callback);

  void AddObserver(NodeRefObserver& observer);
  void RemoveObserver(NodeRefObserver& observer);

 private:
  bool is_null() const { return !impl_; }

  scada::DataValue GetAttribute(scada::AttributeId attribute_id) const;

  std::shared_ptr<NodeRefImpl> impl_;
};

struct NodeRef::Reference {
  NodeRef reference_type;
  NodeRef target;
  bool forward;
};

// NodeRef

inline bool NodeRef::operator==(const NodeRef& other) const {
  if (is_null() || other.is_null())
    return is_null() == other.is_null();
  return impl_ == other.impl_;
}

inline bool NodeRef::operator!=(const NodeRef& other) const {
  return !operator==(other);
}

inline NodeRef& NodeRef::operator=(std::nullptr_t) {
  impl_ = nullptr;
  return *this;
}

inline scada::Variant NodeRef::value() const {
  return data_value().value;
}
