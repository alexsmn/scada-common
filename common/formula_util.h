#pragma once

#include "base/strings/string_piece.h"
#include "core/node_id.h"

bool IsNodeIdFormula(base::StringPiece formula, scada::NodeId& node_id);
std::string MakeNodeIdFormula(const scada::NodeId& id);
