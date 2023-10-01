#include "opc/opc_node_ids.h"

#include "model/nested_node_ids.h"

namespace opc {

namespace {
const char kOpcPathDelimiter = '\\';
const char kOpcCustomItemDelimiter = '.';
}  // namespace

std::optional<ParsedOpcNodeId> ParseOpcNodeId(const scada::NodeId& root_node_id,
                                              const scada::NodeId& node_id) {
  // TODO: Support machine name.

  // SCADA.329!Address
  scada::NodeId parent_id;
  std::string_view nested_name;
  bool ok = IsNestedNodeId(node_id, parent_id, nested_name) &&
            parent_id == root_node_id;
  if (!ok || nested_name.empty()) {
    return std::nullopt;
  }

  auto p = nested_name.find(kOpcPathDelimiter);

  if (p == 0) {
    return std::nullopt;
  }

  // Server node ID consisting from only a prog ID, like `Prog.ID`.
  if (p == nested_name.npos) {
    return ParsedOpcNodeId{.type = OpcNodeType::Server, .prog_id = nested_name};
  }

  auto prog_id = nested_name.substr(0, p);
  auto item_id = nested_name.substr(p + 1);

  // Server node ID ending with a delimiter, like `Prog.ID\`.
  if (item_id.empty()) {
    return ParsedOpcNodeId{.type = OpcNodeType::Server, .prog_id = prog_id};
  }

  // Branch node ID ending with a delimiter, like `Prog.ID\A.B.C\`. So far it
  // includes edge cases like `Prog.ID\\\`.
  if (item_id.back() == kOpcPathDelimiter) {
    return ParsedOpcNodeId{.type = OpcNodeType::Branch,
                           .prog_id = prog_id,
                           .item_id = item_id.substr(0, item_id.size() - 1)};
  }

  return ParsedOpcNodeId{
      .type = OpcNodeType::Item, .prog_id = prog_id, .item_id = item_id};
}

scada::NodeId MakeOpcServerNodeId(const scada::NodeId& root_node_id,
                                  std::string_view prog_id) {
  assert(!root_node_id.is_null());
  assert(!prog_id.empty());

  return MakeNestedNodeId(root_node_id, prog_id);
}

// |item_id| can be empty for server node ID.
scada::NodeId MakeOpcBranchNodeId(const scada::NodeId& root_node_id,
                                  std::string_view prog_id,
                                  std::string_view item_id) {
  assert(!root_node_id.is_null());
  assert(!prog_id.empty());

  if (item_id.empty()) {
    return MakeOpcServerNodeId(root_node_id, prog_id);
  }

  // TODO: Optimize.
  std::string nested_name = std::string{prog_id} + kOpcPathDelimiter +
                            std::string{item_id} + kOpcPathDelimiter;
  return MakeNestedNodeId(root_node_id, nested_name);
}

scada::NodeId MakeOpcItemNodeId(const scada::NodeId& root_node_id,
                                std::string_view prog_id,
                                std::string_view item_id) {
  assert(!root_node_id.is_null());
  assert(!prog_id.empty());
  assert(!item_id.empty());

  // TODO: Optimize.
  std::string nested_name =
      std::string{prog_id} + kOpcPathDelimiter + std::string{item_id};
  return MakeNestedNodeId(root_node_id, nested_name);
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