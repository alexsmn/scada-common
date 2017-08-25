#include "common/formula_util.h"

#include "base/strings/stringprintf.h"

bool IsNodeIdFormula(const base::StringPiece& formula, scada::NodeId& node_id) {
  if (formula.size() < 2)
    return false;
  if (formula[0] != '{' && formula.find('}') != formula.size() - 1)
    return false;

  node_id = scada::NodeId::FromString(formula.substr(1, formula.size() - 2));
  return !node_id.is_null();
}

std::string MakeNodeIdFormula(const scada::NodeId& id) {
  return '{' + id.ToString() + '}';
}

std::string MakeRelativeNodeIdFormula(const scada::NodeId& id, base::StringPiece relative_path) {
  return base::StringPrintf("{{%s}%s}", id.ToString().c_str(), relative_path.as_string().c_str());
}