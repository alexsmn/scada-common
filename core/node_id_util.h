#pragma once

#include "base/strings/string_piece.h"
#include "core/node_id.h"

namespace scada {

bool IsNestedNodeId(const NodeId& node_id, NodeId& parent_id, base::StringPiece& nested_name);
NodeId MakeNestedNodeId(const NodeId& parent_id, const base::StringPiece& nested_name);
bool GetNestedSubName(const NodeId& node_id, const NodeId& nested_id, base::StringPiece& nested_name);

} // namespace scada