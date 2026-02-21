#include "opc/opc_node_ids.h"

#include "base/containers/contains.h"
#include "model/namespaces.h"
#include "model/nested_node_ids.h"
#include "model/opc_node_ids.h"

#include "base/utf_convert.h"
#include <opc_client/core/address.h>

namespace opc {

namespace {
const wchar_t kOpcPathDelimiter = L'\\';
const wchar_t kOpcCustomItemDelimiter = L'.';
}  // namespace

std::optional<opc_client::Address> ParseOpcNodeId(
    const scada::NodeId& node_id) {
  if (node_id.namespace_index() != NamespaceIndexes::OPC ||
      !node_id.is_string() || IsNestedNodeId(node_id)) {
    return std::nullopt;
  }

  return opc_client::Address::Parse(
      UtfConvert<wchar_t>(node_id.string_id()));
}

scada::NodeId MakeOpcNodeId(opc_client::AddressView address) {
  return scada::NodeId{address.ToString(), NamespaceIndexes::OPC};
}

scada::NodeId MakeOpcNodeId(std::wstring_view address) {
  if (auto parsed = opc_client::AddressView::Parse(address)) {
    return MakeOpcNodeId(*parsed);
  } else {
    return {};
  }
}

std::wstring_view GetOpcItemName(std::wstring_view item_id) {
  assert(!item_id.empty());

  // TODO: This relies on the external item ID structure.
  auto p = item_id.find_last_of(kOpcCustomItemDelimiter);
  return p == item_id.npos ? item_id : item_id.substr(p + 1);
}

std::wstring_view GetOpcCustomParentItemId(std::wstring_view item_id) {
  auto p = item_id.find_last_of(kOpcCustomItemDelimiter);
  return p == item_id.npos ? std::wstring_view{} : item_id.substr(0, p);
}

}  // namespace opc