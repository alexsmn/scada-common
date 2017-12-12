#pragma once

#include "base/strings/string_piece.h"
#include "core/node_id.h"

bool IsNestedNodeId(const scada::NodeId& node_id,
                    scada::NodeId& parent_id,
                    base::StringPiece& nested_name);
scada::NodeId MakeNestedNodeId(const scada::NodeId& parent_id,
                               const base::StringPiece& nested_name);
bool GetNestedSubName(const scada::NodeId& node_id,
                      const scada::NodeId& nested_id,
                      base::StringPiece& nested_name);

scada::NodeId NodeIdFromScadaString(base::StringPiece string);
std::string NodeIdToScadaString(const scada::NodeId& node_id);