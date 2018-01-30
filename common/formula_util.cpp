#include "common/formula_util.h"

#include "base/strings/stringprintf.h"
#include "common/node_id_util.h"

bool IsNodeIdFormula(base::StringPiece formula, scada::NodeId& node_id) {
  if (formula.size() < 2)
    return false;
  if (formula[0] != '{' && formula.find('}') != formula.size() - 1)
    return false;

  node_id = NodeIdFromScadaString(formula.substr(1, formula.size() - 2));
  return !node_id.is_null();
}

std::string MakeNodeIdFormula(const scada::NodeId& id) {
  switch (id.type()) {
    case scada::NodeIdType::Numeric:
      return NodeIdToScadaString(id);
    case scada::NodeIdType::String:
      return '{' + NodeIdToScadaString(id) + '}';
    default:
      assert(false);
      return std::string();
  }
}
