#pragma once

#include "opc/opc_address.h"
#include "scada/node_id.h"

namespace opc {

// WARNING: This method must be very performant, as a service locator may invoke
// it for each requested node.
std::optional<OpcAddressView> ParseOpcNodeId(const scada::NodeId& node_id);

// `opc_address` is in format of `Server.ProgId\Branch.ItemId`.
scada::NodeId MakeOpcNodeId(std::string_view opc_address);

// |prog_id| cannot be empty.
inline scada::NodeId MakeOpcServerNodeId(std::string_view prog_id) {
  return MakeOpcNodeId(OpcAddressView::MakeServer(prog_id).ToString());
}

// |prog_id| cannot be empty. If empty |item_id| is provided, then
// `MakeOpcServerNodeId()` is called.
inline scada::NodeId MakeOpcBranchNodeId(std::string_view prog_id,
                                         std::string_view item_id) {
  return MakeOpcNodeId(OpcAddressView::MakeBranch(prog_id, item_id).ToString());
}

// Both |prog_id| and |item_id| cannot be empty.
inline scada::NodeId MakeOpcItemNodeId(std::string_view prog_id,
                                       std::string_view item_id) {
  return MakeOpcNodeId(OpcAddressView::MakeItem(prog_id, item_id).ToString());
}

std::string_view GetOpcItemName(std::string_view item_id);

std::string_view GetOpcCustomParentItemId(std::string_view item_id);

}  // namespace opc
