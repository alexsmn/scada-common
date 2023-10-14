#include "vidicon/vidicon_node_id.h"

#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "model/opc_node_ids.h"
#include "vidicon/data_point_address.h"

#include <boost/locale/encoding_utf.hpp>

namespace vidicon {

scada::NodeId ToNodeId(const DataPointAddress& address) {
  if (address.object_id != 0) {
    return scada::NodeId{static_cast<scada::NumericId>(address.object_id),
                         NamespaceIndexes::VIDICON};
  } else {
    return MakeNestedNodeId(opc::id::OPC, boost::locale::conv::utf_to_utf<char>(
                                              address.opc_address));
  }
}

scada::NodeId ToNodeId(std::wstring_view address) {
  if (auto data_point_address = ParseDataPointAddress(address)) {
    return ToNodeId(*data_point_address);
  } else {
    return scada::NodeId{};
  }
}

}  // namespace vidicon