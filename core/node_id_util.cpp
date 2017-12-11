#include "node_id_util.h"

#include "base/strings/string_util.h"

namespace scada {

bool IsNestedNodeId(const NodeId& node_id, NodeId& parent_id, base::StringPiece& component_name) {
  if (node_id.type() != NodeIdType::String)
    return false;

  // Assert someone doesn't pass us formula.
  const auto& string_id = node_id.string_id();

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

NodeId MakeNestedNodeId(const NodeId& parent_id, const base::StringPiece& component_name) {
  assert(!component_name.empty());
  return NodeId(parent_id.ToString() + '!' + component_name.as_string(), 0);

  /*assert(parent_id.type() == NodeIdType::Numeric);
  assert(!component_name.empty());
  auto string_id = base::StringPrintf("%u.%u/%s", parent_id.namespace_index(), parent_id.numeric_id(), component_name.as_string().c_str());
  return NodeId(std::move(string_id), 0);*/
}

bool GetNestedSubName(const NodeId& node_id, const NodeId& nested_id, base::StringPiece& nested_name) {
  if (node_id.type() != NodeIdType::String)
    return false;

  const auto& string_id = node_id.string_id();
  const std::string nested_string_id = nested_id.ToString();

  if (string_id.size() < nested_string_id.size())
    return false;
  if (!base::StartsWith(string_id, nested_string_id, base::CompareCase::SENSITIVE))
    return false;

  if (string_id.size() == nested_string_id.size())
    return true;

  if (string_id.size() < nested_string_id.size() + 1)
    return false;
  if (string_id[nested_string_id.size()] != '!')
    return false;

  nested_name = string_id.substr(nested_string_id.size() + 1);
  return true;
}

} // namespace scada