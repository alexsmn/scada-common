#pragma once

#include <vector>

#include "core/attribute_ids.h"
#include "core/node_id.h"
#include "core/status.h"
#include "core/variant.h"

namespace scada {

typedef std::pair<NodeId /*prop_type_id*/, Variant /*value*/> NodeProperty;
typedef std::vector<NodeProperty> NodeProperties;

typedef std::pair<NodeId /*ref_type_id*/, NodeId /*ref_node_id*/> NodeReference;
typedef std::vector<NodeReference> NodeReferences;

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

inline bool operator==(const ReferenceDescription& a,
                       const ReferenceDescription& b) {
  return std::tie(a.reference_type_id, a.forward, a.node_id) ==
         std::tie(b.reference_type_id, b.forward, b.node_id);
}

using ReferenceDescriptions = std::vector<ReferenceDescription>;

struct BrowseResult {
  StatusCode status_code;
  std::vector<ReferenceDescription> references;
};

struct ReadValueId {
  NodeId node_id;
  AttributeId attribute_id;
};

inline bool operator==(const ReadValueId& a, const ReadValueId& b) {
  return std::tie(a.node_id, a.attribute_id) ==
         std::tie(b.node_id, b.attribute_id);
}

inline std::ostream& operator<<(std::ostream& stream,
                                const NodeProperty& prop) {
  return stream << "{" << prop.first << ", " << prop.second << "}";
}

inline std::ostream& operator<<(std::ostream& stream,
                                const ReferenceDescription& ref) {
  return stream << "{" << ref.reference_type_id << ", " << ref.forward << ", "
                << ref.node_id << "}";
}

template <class T>
inline std::ostream& operator<<(std::ostream& stream, const std::vector<T>& v) {
  stream << "[";
  for (size_t i = 0; i < v.size(); ++i) {
    stream << v[i];
    if (i != v.size() - 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}

}  // namespace scada
