#pragma once

#include "base/strings/string_piece.h"
#include "core/node_id.h"

scada::NodeId GetFormulaSingleNodeId(std::string_view formula);
std::string MakeNodeIdFormula(const scada::NodeId& id);
