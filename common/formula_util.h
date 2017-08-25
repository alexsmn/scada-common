#pragma once

#include "core/node_id.h"

bool IsNodeIdFormula(const base::StringPiece& formula, scada::NodeId& node_id);
std::string MakeNodeIdFormula(const scada::NodeId& id);
std::string MakeRelativeNodeIdFormula(const scada::NodeId& id, base::StringPiece relative_path);
