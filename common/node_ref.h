#pragma once

#include <memory>

#include "base/optional.h"
#include "core/data_value.h"
#include "core/node_id.h"
#include "core/node_types.h"

class NodeRefService;
struct NodeRefData;

class NodeRef {
 public:
  NodeRef() {}
  NodeRef(std::nullptr_t) {}

  NodeRefService& node_service() const;

  scada::NodeId id() const;

  bool fetched() const;

  base::Optional<scada::NodeClass> node_class() const;
  std::string browse_name() const;
  base::string16 display_name() const;
  NodeRef type_definition() const;
  NodeRef supertype() const;
  NodeRef data_type() const;

  // Includes components and properies.
  std::vector<NodeRef> aggregates() const;
  std::vector<NodeRef> components() const;
  std::vector<NodeRef> properties() const;

  struct Reference;
  using References = std::vector<Reference>;

  // Non-herarchical references.
  std::vector<Reference> references() const;

  scada::DataValue data_value() const;
  scada::Variant value() const;

  explicit operator bool() const { return !is_null(); }

  bool operator==(const NodeRef& other) const;
  bool operator!=(const NodeRef& other) const;

  NodeRef& operator=(std::nullptr_t);
  bool operator==(std::nullptr_t) const { return is_null(); }
  bool operator!=(std::nullptr_t) const { return !is_null(); }

  NodeRef operator[](base::StringPiece aggregate_name) const;
  NodeRef operator[](const scada::NodeId& aggregate_declaration_id) const;

  NodeRef target(const scada::NodeId& reference_type_id) const;
  std::vector<NodeRef> targets(const scada::NodeId& reference_type_id) const;

  NodeRef GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const;

 private:
  explicit NodeRef(std::shared_ptr<NodeRefData> data);

  bool is_null() const { return !data_; }

  std::shared_ptr<NodeRefData> data_;

  friend class NodeRefService;
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
  return data_ == other.data_;
}

inline bool NodeRef::operator!=(const NodeRef& other) const {
  return !operator==(other);
}

inline NodeRef& NodeRef::operator=(std::nullptr_t) {
  data_ = nullptr;
  return *this;
}

inline scada::Variant NodeRef::value() const {
  return data_value().value;
}
