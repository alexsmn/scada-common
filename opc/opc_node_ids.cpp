#include "opc/opc_node_ids.h"

#include "base/containers/contains.h"
#include "model/namespaces.h"
#include "model/nested_node_ids.h"
#include "model/opc_node_ids.h"

namespace opc {

namespace {
const char kOpcPathDelimiter = '\\';
const char kOpcCustomItemDelimiter = '.';
}  // namespace

std::optional<OpcAddressView> ParseOpcNodeId(const scada::NodeId& node_id) {
  if (node_id.namespace_index() != NamespaceIndexes::OPC ||
      !node_id.is_string() || IsNestedNodeId(node_id)) {
    return std::nullopt;
  }

  return OpcAddressView::Parse(node_id.string_id());
}

scada::NodeId MakeOpcNodeId(std::string_view address) {
  return scada::NodeId{std::string{address}, NamespaceIndexes::OPC};
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