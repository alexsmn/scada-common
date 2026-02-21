#include "common/formula_util.h"

#include "base/strings/strcat.h"
#include "common/scada_expression.h"
#include "model/node_id_util.h"
#include "model/static_types.h"

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

std::string GetFormulaSingleName(std::string_view formula) {
  std::string name;
  if (!ScadaExpression::IsSingleName(formula, name))
    return {};

  if (auto str = Unquote(name, '{', '}'))
    return std::string{*str};

  return name;
}

scada::NodeId GetFormulaSingleNodeId(std::string_view formula) {
  auto name = GetFormulaSingleName(formula);
  return name.empty() ? scada::NodeId{} : NodeIdFromScadaString(name);
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

std::string_view GetParentGroupChannelPath(std::string_view path) {
  bool matches = path.starts_with(cfg::kDataGroupDevicePlaceholder);
  if (!matches)
    return {};

  path = path.substr(std::string_view{cfg::kDataGroupDevicePlaceholder}.size());
  if (path.empty() || path[0] != '!')
    return {};

  return path.substr(1);
}
