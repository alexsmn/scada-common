#include "opc/opc_node_ids.h"

#include "model/nested_node_ids.h"
#include "model/opc_node_ids.h"

namespace opc {

namespace {
const char kOpcPathDelimiter = '\\';
const char kOpcCustomItemDelimiter = '.';
}  // namespace

std::optional<OpcAddressView> ParseOpcNodeId(const scada::NodeId& node_id) {
  // TODO: Support machine name.

  // SCADA.329!Address
  scada::NodeId parent_id;
  std::string_view nested_name;
  bool ok =
      IsNestedNodeId(node_id, parent_id, nested_name) && parent_id == id::OPC;
  if (!ok) {
    return std::nullopt;
  }

  return OpcAddressView::Parse(nested_name);
}

scada::NodeId MakeOpcNodeId(std::string_view address) {
  return MakeNestedNodeId(id::OPC, address);
}

std::string_view GetOpcItemName(std::string_view item_id) {
  assert(!item_id.empty());

  // TODO: This relies on the external item ID structure.
  auto p = item_id.find_last_of(kOpcCustomItemDelimiter);
  return p == item_id.npos ? item_id : item_id.substr(p + 1);
}

std::string_view GetOpcCustomParentItemId(std::string_view item_id) {
  auto p = item_id.find_last_of(kOpcCustomItemDelimiter);
  return p == item_id.npos ? std::string_view{} : item_id.substr(0, p);
}

}  // namespace opc