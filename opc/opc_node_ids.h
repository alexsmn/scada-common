#pragma once

#include "scada/node_id.h"

#include <optional>

namespace opc {

struct OpcItemId {
  std::string machine_name;
  std::string prog_id;
  std::string item_id;
};

enum class OpcNodeType { Server, Branch, Item };

struct ParsedOpcNodeId {
  OpcNodeType type = OpcNodeType::Item;
  std::string_view machine_name;
  std::string_view prog_id;
  // Never empty for item nodes.
  std::string_view item_id;
};

// WARNING: This method must be very performant, as a service locator may invoke
// it for each requested node.
std::optional<ParsedOpcNodeId> ParseOpcNodeId(const scada::NodeId& root_node_id,
                                              const scada::NodeId& node_id);

// |prog_id| cannot be empty.
scada::NodeId MakeOpcServerNodeId(const scada::NodeId& root_node_id,
                                  std::string_view prog_id);

// |prog_id| cannot be empty. If empty |item_id| is provided, then
// `MakeOpcServerNodeId()` is called.
scada::NodeId MakeOpcBranchNodeId(const scada::NodeId& root_node_id,
                                  std::string_view prog_id,
                                  std::string_view item_id);

scada::NodeId MakeOpcItemNodeId(const scada::NodeId& root_node_id,
                                std::string_view prog_id,
                                std::string_view item_id);

std::string_view GetOpcItemName(std::string_view item_id);

std::string_view GetOpcCustomParentItemId(std::string_view item_id);

}  // namespace opc
