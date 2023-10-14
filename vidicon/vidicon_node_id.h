#pragma once

#include "scada/node_id.h"
#include "vidicon/data_point_address.h"

#include <optional>

namespace vidicon {

extern const scada::NodeId kVidiconRootNodeId;
extern const scada::NodeId kVidiconRootFileNodeId;

scada::NodeId ToNodeId(const DataPointAddress& address);

// Returns the null node ID if the address cannot be parsed.
scada::NodeId ToNodeId(std::wstring_view address);

bool IsVidiconNodeId(const scada::NodeId& node_id);
std::optional<VidiconObjectId> GetVidiconObjectId(const scada::NodeId& node_id);
scada::NodeId MakeVidiconNodeId(VidiconObjectId object_id);

bool IsVidiconFileNodeId(const scada::NodeId& node_id);
scada::NodeId MakeVidiconFileNodeId(VidiconFileId vidicon_file_id);

}  // namespace vidicon