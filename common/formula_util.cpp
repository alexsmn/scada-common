#include "common/formula_util.h"

#include "base/strings/strcat.h"
#include "common/scada_expression.h"
#include "common/node_id_util.h"

bool IsNodeIdFormula(base::StringPiece formula, scada::NodeId& node_id) {
  std::string item_name;
  if (!ScadaExpression::IsSingleName(formula, item_name))
    return false;

  if (item_name.size() >= 2 && item_name.front() == '{' && item_name.back() == '}') {
    const auto& s = base::StringPiece{item_name}.substr(1, item_name.size() - 2);
    node_id = NodeIdFromScadaString(s);
    return !node_id.is_null();
  }

  node_id = NodeIdFromScadaString(item_name);
  return !node_id.is_null();
}

std::string MakeNodeIdFormula(const scada::NodeId& id) {
  switch (id.type()) {
    case scada::NodeIdType::Numeric:
      return NodeIdToScadaString(id);
    case scada::NodeIdType::String:
      return base::StrCat({"{", NodeIdToScadaString(id), "}"});
    default:
      assert(false);
      return std::string();
  }
}
