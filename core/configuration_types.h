#pragma once

#include <vector>

#include "core/attribute_ids.h"
#include "core/node_class.h"
#include "core/node_id.h"
#include "core/status.h"
#include "core/variant.h"

namespace scada {

using ReadValueId = std::pair<NodeId, AttributeId>;

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

using ReferenceDescriptions = std::vector<ReferenceDescription>;

struct BrowseResult {
  StatusCode status_code;
  std::vector<ReferenceDescription> references;
};

} // namespace scada