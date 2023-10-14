#pragma once

#include "scada/node_id.h"

namespace vidicon {

struct DataPointAddress;

scada::NodeId ToNodeId(const DataPointAddress& address);

// Returns the null node ID if the address cannot be parsed.
scada::NodeId ToNodeId(std::wstring_view address);

}  // namespace vidicon