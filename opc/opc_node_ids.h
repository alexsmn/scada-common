#pragma once

#include "scada/node_id.h"

#include <opc_client/core/address.h>
#include <optional>

namespace opc {

// WARNING: This method must be very performant, as a service locator may invoke
// it for each requested node.
std::optional<opc_client::AddressView> ParseOpcNodeId(
    const scada::NodeId& node_id);

scada::NodeId MakeOpcNodeId(opc_client::AddressView address);

// Parses `address` and makes a node ID.
// Returns null node ID if `address` cannot be parsed.
scada::NodeId MakeOpcNodeId(std::wstring_view address);

// |prog_id| cannot be empty.
inline scada::NodeId MakeOpcServerNodeId(std::wstring_view prog_id) {
  return MakeOpcNodeId(opc_client::AddressView::MakeServer(prog_id));
}

// |prog_id| cannot be empty. If empty |item_id| is provided, then
// `MakeOpcServerNodeId()` is called.
inline scada::NodeId MakeOpcBranchNodeId(std::wstring_view prog_id,
                                         std::wstring_view item_id) {
  return MakeOpcNodeId(opc_client::AddressView::MakeBranch(prog_id, item_id));
}

// Both |prog_id| and |item_id| cannot be empty.
inline scada::NodeId MakeOpcItemNodeId(std::wstring_view prog_id,
                                       std::wstring_view item_id) {
  return MakeOpcNodeId(opc_client::AddressView::MakeItem(prog_id, item_id));
}

std::wstring_view GetOpcItemName(std::wstring_view item_id);

std::wstring_view GetOpcCustomParentItemId(std::wstring_view item_id);

}  // namespace opc
