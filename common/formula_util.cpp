#include "common/formula_util.h"

#include "common/scada_expression.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "model/static_types.h"
#include "scada/node_id_string.h"

#include <format>
#include <functional>
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
      return std::format("{{{}}}", NodeIdToScadaString(id));
    default:
      // Node IDs may come from external configuration; unsupported ID types
      // yield an empty formula instead of panicking.
      return std::string();
  }
}

std::string MakeStoredNodeIdFormula(
    const scada::NodeId& id,
    const scada::NamespaceUriForIndex& uri_for_index) {
  std::string text = scada::NodeIdToStoredString(id, uri_for_index);
  if (!scada::ParseNodeIdString(text))
    return MakeNodeIdFormula(id);
  if (text.find('}') != std::string::npos)
    return std::string();
  return std::format("{{{}}}", text);
}

namespace {

// Rewrites one node reference, or answers nullopt to keep it as written.
using ReferenceRewriter =
    std::function<std::optional<std::string>(std::string_view reference)>;

// The single name `text` consists of, unbraced, or nullopt. The lexer throws
// on text it cannot read, which is then not a single name either.
std::optional<std::string> UnbracedSingleName(std::string_view text) {
  std::string name;
  try {
    if (!ScadaExpression::IsSingleName(text, name))
      return std::nullopt;
  } catch (...) {
    return std::nullopt;
  }
  if (Unquote(name, '{', '}'))
    return std::nullopt;
  return name;
}

std::string RewriteReferences(std::string_view formula,
                              const ReferenceRewriter& rewrite) {
  std::string_view prefix;
  std::string_view body = formula;
  if (body.starts_with('=')) {
    prefix = body.substr(0, 1);
    body.remove_prefix(1);
  }

  // A whole reference: the Part 6 form, which the lexer does not read as one
  // name, or a single unbraced name.
  std::optional<std::string> whole;
  if (scada::ParseNodeIdString(body))
    whole = std::string{body};
  else
    whole = UnbracedSingleName(body);
  if (whole) {
    if (std::optional<std::string> rewritten = rewrite(*whole))
      return std::format("{}{}", prefix, *rewritten);
    return std::string{formula};
  }

  // Braced operands. The lexer reads a braced name up to the first `}`.
  std::string result{prefix};
  std::size_t pos = 0;
  while (pos < body.size()) {
    const std::size_t open = body.find('{', pos);
    const std::size_t close =
        open == std::string_view::npos ? open : body.find('}', open + 1);
    if (close == std::string_view::npos) {
      result += body.substr(pos);
      break;
    }
    result += body.substr(pos, open - pos);
    const std::string_view reference = body.substr(open + 1, close - open - 1);
    std::optional<std::string> rewritten = rewrite(reference);
    if (rewritten && rewritten->find('}') == std::string::npos)
      result += std::format("{{{}}}", *rewritten);
    else
      result += body.substr(open, close - open + 1);
    pos = close + 1;
  }
  return result;
}

// Whether the model has a short name for `index`, so that NodeIdToScadaString
// already writes it in a form any session reads.
bool HasShortName(scada::NamespaceIndex index) {
  return !scada::GetNamespaceName(index).empty();
}

}  // namespace

std::string FormulaReferencesToStored(
    std::string_view formula,
    const scada::NamespaceUriForIndex& uri_for_index) {
  return RewriteReferences(
      formula, [&](std::string_view reference) -> std::optional<std::string> {
        if (scada::ParseNodeIdString(reference))
          return std::nullopt;
        const scada::NodeId node_id = NodeIdFromScadaString(reference);
        if (node_id.is_null() || HasShortName(node_id.namespace_index()))
          return std::nullopt;
        std::string stored =
            scada::NodeIdToStoredString(node_id, uri_for_index);
        if (!scada::ParseNodeIdString(stored))
          return std::nullopt;
        return stored;
      });
}

std::string FormulaReferencesFromStored(
    std::string_view formula,
    const scada::NamespaceIndexForUri& index_for_uri) {
  return RewriteReferences(
      formula, [&](std::string_view reference) -> std::optional<std::string> {
        std::optional<scada::ParsedNodeIdString> parsed =
            scada::ParseNodeIdString(reference);
        if (!parsed || !parsed->namespace_uri)
          return std::nullopt;
        std::optional<scada::NodeId> node_id =
            scada::NodeIdFromNodeIdString(reference, index_for_uri);
        if (!node_id || node_id->is_null() ||
            HasShortName(node_id->namespace_index())) {
          return std::nullopt;
        }
        std::string session = NodeIdToScadaString(*node_id);
        if (session.empty())
          return std::nullopt;
        return session;
      });
}

std::string_view GetParentGroupChannelPath(std::string_view path) {
  bool matches = path.starts_with(scada::cfg::kDataGroupDevicePlaceholder);
  if (!matches)
    return {};

  path = path.substr(
      std::string_view{scada::cfg::kDataGroupDevicePlaceholder}.size());
  if (path.empty() || path[0] != '!')
    return {};

  return path.substr(1);
}
