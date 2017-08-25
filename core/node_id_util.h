#pragma once

#include "base/strings/string_piece.h"

namespace scada {

class NodeId;

bool IsNestedNodeId(const NodeId& node_id, NodeId& parent_id, base::StringPiece& nested_name);
NodeId MakeNestedNodeId(const NodeId& parent_id, const base::StringPiece& nested_name);
NodeId MakeNestedNodeId(const NodeId& parent_id, const NodeId& component_declaration_id);

bool IsBaseTypeDefinitionId(const NodeId& id);

} // namespace scada
