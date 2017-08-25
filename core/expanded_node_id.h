#pragma once

#include "core/node_id.h"

#include <string>

namespace scada {

class ExpandedNodeId {
 public:
  ExpandedNodeId(NodeId node_id, std::string namespace_uri, unsigned server_index);

 private:
  const NodeId node_id_;
  const std::string namespace_uri_;
  const unsigned server_index_;
};

inline ExpandedNodeId::ExpandedNodeId(NodeId node_id, std::string namespace_uri, unsigned server_index)
    : node_id_{std::move(node_id)},
      namespace_uri_{std::move(namespace_uri)},
      server_index_{server_index} {
}

} // namespace scada
