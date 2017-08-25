#include "core/node_id_util.h"

#include "base/strings/string_util.h"
#include "core/node_id.h"
#include "core/standard_node_ids.h"

namespace scada {

// TODO: Move to server.
scada::NodeId MakeNestedNodeId(const NodeId& parent_id, const base::StringPiece& component_name) {
  assert(!component_name.empty());
  assert(base::IsStringASCII(component_name));
  return scada::NodeId(parent_id.ToString() + '!' + component_name.as_string(), 0);
}

scada::NodeId MakeNestedNodeId(const NodeId& parent_id, const NodeId& component_declaration_id) {
  assert(parent_id.type() == scada::NodeIdType::Numeric);
  assert(component_declaration_id.type() == scada::NodeIdType::Numeric);
  return scada::NodeId();
}

bool IsNestedNodeId(const NodeId& node_id, NodeId& parent_id, base::StringPiece& component_name) {
  if (node_id.type() != NodeIdType::String)
    return false;

  // Assert someone didn't pass a formula.
  base::StringPiece string_id = node_id.string_id();
  assert(string_id.find('{') == std::string::npos);
  assert(string_id.find('}') == std::string::npos);

  auto p = string_id.find('!');
  if (p == std::string::npos)
    return false;

  auto node_parent_id = NodeId::FromString(string_id.substr(0, p));
  if (node_parent_id.is_null())
    return false;

  parent_id = node_parent_id;
  component_name = string_id.substr(p + 1);
  return true;
}

bool IsBaseTypeDefinitionId(const NodeId& id) {
  return id == OpcUaId_BaseDataType ||
         id == OpcUaId_BaseObjectType ||
         id == OpcUaId_BaseVariableType;
}

} // namespace scada
