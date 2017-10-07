#include "core/node_id_util.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "core/node_id.h"
#include "core/standard_node_ids.h"

namespace scada {

// TODO: Move to server.
scada::NodeId MakeNestedNodeId(const NodeId& parent_id, const base::StringPiece& component_name) {
  assert(!parent_id.is_null());
  assert(!component_name.empty());
  assert(base::IsStringASCII(component_name));
  auto string_id = base::StringPrintf("[%s][%s]", parent_id.ToString().c_str(), component_name.as_string().c_str());
  return {std::move(string_id), 0};
}

scada::NodeId MakeNestedNodeId(const NodeId& parent_id, const NodeId& component_declaration_id) {
  assert(!component_declaration_id.is_null());
  return MakeNestedNodeId(parent_id, component_declaration_id.ToString());
}

bool IsNestedNodeId(const NodeId& node_id, NodeId& parent_id, base::StringPiece& component_name) {
  // Parses string like "{node_id}{component_name}"

  if (node_id.type() != NodeIdType::String)
    return false;

  base::StringPiece string_id = node_id.string_id();

  if (string_id.size() < 4)
    return false;

  if (string_id.front() != '[' || string_id.back() != ']')
    return false;

  size_t index = string_id.find("][");
  if (index == base::StringPiece::npos)
    return false;

  if (string_id.substr(index + 2).find("][") != base::StringPiece::npos)
    return false;

  auto node_parent_id = NodeId::FromString(string_id.substr(1, index - 1));
  if (node_parent_id.is_null())
    return false;

  parent_id = node_parent_id;
  component_name = string_id.substr(index + 2);
  return true;
}

bool IsBaseTypeDefinitionId(const NodeId& id) {
  return id == id::BaseDataType ||
         id == id::BaseObjectType ||
         id == id::BaseVariableType;
}

} // namespace scada
