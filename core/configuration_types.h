#pragma once

#include <vector>

#include "core/attribute_ids.h"
#include "core/node_id.h"
#include "core/node_types.h"
#include "core/status.h"
#include "core/variant.h"

namespace scada {

using ReadValueId = std::pair<NodeId, AttributeId>;

typedef std::pair<NodeId /*prop_type_id*/, Variant /*value*/> NodeProperty;
typedef std::vector<NodeProperty> NodeProperties;

typedef std::pair<NodeId /*ref_type_id*/, NodeId /*ref_node_id*/> NodeReference;
typedef std::vector<NodeReference> NodeReferences;

class AttributeSet {
 public:
  void Add(AttributeId id) { bits_ |= 1 << static_cast<unsigned>(id); }
  void Remove(AttributeId id) { bits_ &= ~(1 << static_cast<unsigned>(id)); }

  bool has(AttributeId id) const { return (bits_ & (1 << static_cast<unsigned>(id))) != 0; }

  bool empty() const { return bits_ == 0; }

 private:
  unsigned bits_ = 0;
};

class NodeAttributes : public AttributeSet  {
 public:
  std::string& browse_name() { assert(has(OpcUa_Attributes_BrowseName)); return browse_name_; }
  const std::string& browse_name() const { assert(has(OpcUa_Attributes_BrowseName)); return browse_name_; }
  NodeAttributes& set_browse_name(std::string value) {
    browse_name_ = std::move(value);
    Add(OpcUa_Attributes_BrowseName);
    return *this;
  }

  base::string16& display_name() { assert(has(OpcUa_Attributes_DisplayName)); return display_name_; }
  const base::string16& display_name() const { assert(has(OpcUa_Attributes_DisplayName)); return display_name_; }
  NodeAttributes& set_display_name(base::string16 value) {
    display_name_ = std::move(value);
    Add(OpcUa_Attributes_DisplayName);
    return *this;
  }

  const NodeId& data_type_id() const { assert(has(OpcUa_Attributes_DataType)); return data_type_id_; }
  NodeAttributes& set_data_type_id(NodeId data_type_id) {
    data_type_id_ = std::move(data_type_id);
    Add(OpcUa_Attributes_DataType);
    return *this;
  }

  Variant& value() { assert(has(OpcUa_Attributes_Value)); return value_; }
  const Variant& value() const { assert(has(OpcUa_Attributes_Value)); return value_; }
  NodeAttributes& set_value(Variant value) {
    value_ = std::move(value);
    Add(OpcUa_Attributes_Value);
    return *this;
  }

 private:
  std::string browse_name_;
  base::string16 display_name_;
  NodeId data_type_id_;
  Variant value_;
};

enum class BrowseDirection {
  Forward = 0,
  Inverse = 1,
  Both = 2,
};

struct BrowseDescription {
  NodeId node_id;
  BrowseDirection direction;
  NodeId reference_type_id;
  bool include_subtypes;
};

struct ReferenceDescription {
  NodeId reference_type_id;
  bool forward;
  NodeId node_id;
};

using ReferenceDescriptions = std::vector<ReferenceDescription>;

struct BrowseResult {
  StatusCode status_code;
  std::vector<ReferenceDescription> references;
};

} // namespace scada