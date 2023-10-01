#pragma once

#include "scada/node_id.h"

#include <optional>

namespace opc {

struct OpcItemId {
  std::string machine_name;
  std::string prog_id;
  std::string item_id;
};

struct ParsedOpcNodeId {
  std::string_view machine_name;
  std::string_view prog_id;

  // `item_id` is empty for server node ID.
  std::string_view item_id;
};

// WARNING: This method must be very performant, as a service locator may invoke
// it for each requested node.
std::optional<ParsedOpcNodeId> ParseOpcNodeId(const scada::NodeId& root_node_id,
                                              const scada::NodeId& node_id);

// |item_id| can be empty for server node ID.
scada::NodeId MakeOpcBranchNodeId(const scada::NodeId& root_node_id,
                                  std::string_view prog_id,
                                  std::string_view item_id);

scada::NodeId MakeOpcServerNodeId(const scada::NodeId& root_node_id,
                                  std::string_view prog_id);

scada::NodeId MakeOpcItemNodeId(const scada::NodeId& root_node_id,
                                std::string_view prog_id,
                                std::string_view item_id);

std::string_view GetOpcItemName(std::string_view item_id);

std::string_view GetOpcCustomParentItemId(std::string_view item_id);

bool IsBranchOpcItemId(std::string_view item_id);

}  // namespace opc
