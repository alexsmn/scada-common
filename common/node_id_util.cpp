#include "node_id_util.h"

#include "base/strings/string_util.h"

bool IsNestedNodeId(const scada::NodeId& node_id,
                    scada::NodeId& parent_id,
                    base::StringPiece& component_name) {
  const auto* string_id = node_id.string_id();
  if (!string_id)
    return false;

  auto p = string_id->find('!');
  if (p == std::string::npos)
    return false;

  auto node_parent_id = scada::NodeId::FromString(string_id->substr(0, p));
  if (node_parent_id.is_null())
    return false;

  parent_id = node_parent_id;
  component_name = string_id->substr(p + 1);
  return true;
}

scada::NodeId MakeNestedNodeId(const scada::NodeId& parent_id,
                               const base::StringPiece& component_name) {
  assert(!component_name.empty());
  return scada::NodeId(parent_id.ToString() + '!' + component_name.as_string(),
                       0);
}

bool GetNestedSubName(const scada::NodeId& node_id,
                      const scada::NodeId& nested_id,
                      base::StringPiece& nested_name) {
  const auto* string_id = node_id.string_id();
  if (!string_id)
    return false;

  const std::string nested_string_id = nested_id.ToString();

  if (string_id->size() < nested_string_id.size())
    return false;
  if (!base::StartsWith(*string_id, nested_string_id,
                        base::CompareCase::SENSITIVE))
    return false;

  if (string_id->size() == nested_string_id.size())
    return true;

  if (string_id->size() < nested_string_id.size() + 1)
    return false;
  if ((*string_id)[nested_string_id.size()] != '!')
    return false;

  nested_name = string_id->substr(nested_string_id.size() + 1);
  return true;
}

std::string NodeIdToScadaString(const scada::NodeId& node_id) {
  return node_id.ToString();
}

scada::NodeId NodeIdFromScadaString(base::StringPiece scada_string) {
  return scada::NodeId::FromString(scada_string);
}