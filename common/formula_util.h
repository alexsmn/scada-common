#pragma once

#include "core/node_id.h"

#include <string_view>

bool IsNodeIdFormula(std::string_view formula, scada::NodeId& node_id);
std::string MakeNodeIdFormula(const scada::NodeId& id);
