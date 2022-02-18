#include "common/formula_util.h"

#include "base/strings/strcat.h"
#include "common/scada_expression.h"
#include "model/node_id_util.h"

#include <optional>

std::optional<std::string_view> Unquote(std::string_view str,
                                         char left_quote,
                                         char right_quote) {
  if (str.size() >= 2 && str.front() == left_quote &&
      str.back() == right_quote) {
    return str.substr(1, str.size() - 2);
  } else {
    return std::nullopt;
  }
}

scada::NodeId GetFormulaSingleNodeId(std::string_view formula) {
  std::string name;
  if (!ScadaExpression::IsSingleName(formula, name))
    return {};

  if (auto str = Unquote(name, '{', '}'))
    return NodeIdFromScadaString(*str);

  return NodeIdFromScadaString(name);
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
