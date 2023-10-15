#include "vidicon/vidicon_node_id.h"

#include "model/data_items_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "opc/opc_node_ids.h"
#include "vidicon/data_point_address.h"

#include <boost/locale/encoding_utf.hpp>

namespace vidicon {

const scada::NodeId kVidiconRootNodeId = data_items::id::DataItems;
const scada::NodeId kVidiconRootFileNodeId = filesystem::id::FileSystem;

scada::NodeId ToNodeId(const DataPointAddress& address) {
  if (address.object_id != 0) {
    return scada::NodeId{static_cast<scada::NumericId>(address.object_id),
                         NamespaceIndexes::VIDICON};
  } else {
    return opc::MakeOpcNodeId(
        boost::locale::conv::utf_to_utf<char>(address.opc_address));
  }
}

scada::NodeId ToNodeId(std::wstring_view address) {
  if (auto data_point_address = ParseDataPointAddress(address)) {
    return ToNodeId(*data_point_address);
  } else {
    return scada::NodeId{};
  }
}

bool IsVidiconNodeId(const scada::NodeId& node_id) {
  return node_id.namespace_index() == NamespaceIndexes::VIDICON ||
         node_id == kVidiconRootNodeId;
}

std::optional<VidiconObjectId> GetVidiconObjectId(
    const scada::NodeId& node_id) {
  if (node_id.namespace_index() == NamespaceIndexes::VIDICON &&
      node_id.is_numeric()) {
    return static_cast<VidiconObjectId>(node_id.numeric_id());
  } else {
    return std::nullopt;
  }
}

scada::NodeId MakeVidiconNodeId(VidiconObjectId object_id) {
  return object_id == kRootVidiconObjectId
             ? kVidiconRootNodeId
             : scada::NodeId{object_id, NamespaceIndexes::VIDICON};
}

bool IsVidiconFileNodeId(const scada::NodeId& node_id) {
  return node_id.namespace_index() == NamespaceIndexes::VIDICON_FILE ||
         node_id == kVidiconRootFileNodeId;
}

scada::NodeId MakeVidiconFileNodeId(VidiconFileId vidicon_file_id) {
  return vidicon_file_id == kRootVidiconFileId
             ? kVidiconRootFileNodeId
             : scada::NodeId{vidicon_file_id, NamespaceIndexes::VIDICON_FILE};
}

}  // namespace vidicon