#pragma once

#include "scada/node_id.h"

std::string GetFormulaSingleName(std::string_view formula);
scada::NodeId GetFormulaSingleNodeId(std::string_view formula);
std::string MakeNodeIdFormula(const scada::NodeId& id);

std::string_view GetParentGroupChannelPath(std::string_view path);
