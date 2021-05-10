#pragma once

#include "core/node_id.h"

#include <string_view>

scada::NodeId GetFormulaSingleNodeId(std::string_view formula);
std::string MakeNodeIdFormula(const scada::NodeId& id);
