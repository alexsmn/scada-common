#include "node_id_util.h"

#include <algorithm>
#include <format>

#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "scada/node_id_log.h"
#include <boost/algorithm/string/predicate.hpp>

#if defined(SCADA_USE_CORE_MODULE)
// Modules-pilot consumer (SCADA_CXX_MODULES=ON): base/scada names come from
// the scada.core facade (which export-imports scada.base). The import sits
// after the textual includes because the reverse order trips an AppleClang
// 21 declaration-merging bug in libc++.
import scada.core;
#else
#include "base/check.h"
#include "base/format.h"
#endif

namespace {

const std::pair<std::string_view, scada::NodeId> kPredefinedScadaStrings[] = {
    {"Server!CPU", data_items::id::Statistics_TotalCPUUsage},
    {"Server!Mem", data_items::id::Statistics_TotalMemoryUsage},
    {"Server!PCPU", data_items::id::Statistics_ServerCPUUsage},
    {"Server!PMem", data_items::id::Statistics_ServerMemoryUsage},
};

scada::NodeId NodeIdFromPredefinedScadaString(std::string_view scada_string) {
  auto i = std::ranges::find(kPredefinedScadaStrings, scada_string,
                             [](const auto& p) { return p.first; });
  return i != std::cend(kPredefinedScadaStrings) ? i->second : scada::NodeId{};
}

std::string_view NodeIdToPredefinedScadaString(const scada::NodeId& node_id) {
  auto i = std::ranges::find(kPredefinedScadaStrings, node_id,
                             [](const auto& p) { return p.second; });
  return i != std::cend(kPredefinedScadaStrings) ? i->first
                                                 : std::string_view{};
}

}  // namespace

namespace scada {

bool IsNestedNodeId(const scada::NodeId& node_id,
                    scada::NodeId& parent_id,
                    std::string_view& nested_name) {
  if (node_id.type() != scada::NodeIdType::String)
    return false;

  const std::string_view string_id = node_id.string_id();
  const auto p = string_id.find_last_of('!');
  if (p == std::string::npos)
    return false;

  const bool is_topmost_parent = string_id.find('!') == p;
  std::string_view parent_string_id = string_id.substr(0, p);
  auto parent_namespace_index = node_id.namespace_index();

  // Parent can have optional namespace specification.
  if (auto np = parent_string_id.find(':'); np != parent_string_id.npos) {
    int pni = FindNamespaceIndexByName(parent_string_id.substr(0, np));
    if (pni == -1)
      return false;
    parent_namespace_index = static_cast<scada::NamespaceIndex>(pni);
    parent_string_id = parent_string_id.substr(np + 1);
  }

  // The topmost parent node ID can be a number or a string. Next levels are
  // always strings, since they have to contain the `!` separator.
  if (is_topmost_parent) {
    scada::NumericId parent_numeric_id = 0;
    if (Parse(parent_string_id, parent_numeric_id)) {
      parent_id = scada::NodeId{parent_numeric_id, parent_namespace_index};
    } else {
      parent_id =
          scada::NodeId{std::string{parent_string_id}, parent_namespace_index};
    }
  } else {
    parent_id =
        scada::NodeId{scada::String{parent_string_id}, parent_namespace_index};
  }

  nested_name = string_id.substr(p + 1);

  // Check missing nested name, e.g. `123!`.
  return !nested_name.empty();
}

bool GetRootNestedNodeId(const scada::NodeId& node_id,
                         scada::NodeId& parent_id,
                         std::string_view& nested_name) {
  if (!IsNestedNodeId(node_id, parent_id, nested_name))
    return false;

  scada::NodeId next_parent_id;
  std::string_view next_nested_name;
  while (IsNestedNodeId(parent_id, next_parent_id, next_nested_name)) {
    parent_id = std::move(next_parent_id);
    nested_name = next_nested_name;
  }

  return true;
}

scada::NodeId MakeNestedNodeId(const scada::NodeId& parent_id,
                               std::string_view nested_name) {
  return MakeNestedNodeId(parent_id, nested_name, parent_id.namespace_index());
}

scada::NodeId MakeNestedNodeId(const scada::NodeId& parent_id,
                               std::string_view nested_name,
                               scada::NamespaceIndex namespace_index) {
  base::Check(!parent_id.is_null());
  base::Check(!nested_name.empty());

  if (parent_id.is_null() || nested_name.empty()) {
    return {};
  }

  auto parent_namespace = namespace_index != parent_id.namespace_index()
                              ? std::format("{}:", parent_id.namespace_index())
                              : std::string{};
  auto parent_identifier = parent_id.type() == scada::NodeIdType::Numeric
                               ? std::format("{}", parent_id.numeric_id())
                               : std::string{parent_id.string_id()};
  auto identifier =
      std::format("{}{}!{}", parent_namespace, parent_identifier, nested_name);

  return scada::NodeId{std::move(identifier), namespace_index};
}

bool GetNestedSubName(const scada::NodeId& node_id,
                      const scada::NodeId& nested_id,
                      std::string_view& nested_name) {
  // For backward compatibility.
  if (node_id == nested_id) {
    nested_name = {};
    return true;
  }

  scada::NodeId parent_id;
  if (!IsNestedNodeId(node_id, parent_id, nested_name))
    return false;

  return parent_id == nested_id;
}

std::string NodeIdToScadaString(const scada::NodeId& node_id) {
  if (auto predefined_scada_string = NodeIdToPredefinedScadaString(node_id);
      !predefined_scada_string.empty())
    return std::string{predefined_scada_string};

  std::string namespace_name =
      std::string{GetNamespaceName(node_id.namespace_index())};
  if (namespace_name.empty()) {
    namespace_name = std::format("NS{}", node_id.namespace_index());
  }

  std::string identifier;

  switch (node_id.type()) {
    case scada::NodeIdType::Numeric:
      identifier = std::format("{}", node_id.numeric_id());
      break;

    case scada::NodeIdType::String:
      identifier = node_id.string_id();
      break;

    default:
      // Opaque identifiers have no scada-string form; callers treat an
      // empty result as unsupported.
      return {};
  }

  return std::format("{}.{}", namespace_name, identifier);
}

scada::NodeId NodeIdFromScadaString(std::string_view scada_string) {
  if (auto node_id = NodeIdFromPredefinedScadaString(scada_string);
      !node_id.is_null())
    return node_id;

  const auto p = scada_string.find('.');
  if (p == std::string_view::npos)
    return scada::NodeId{};

  const auto namespace_name = scada_string.substr(0, p);
  int namespace_index = FindNamespaceIndexByName(namespace_name);
  if (namespace_index == -1) {
    if (!boost::istarts_with(namespace_name, "NS")) {
      return scada::NodeId{};
    }
    if (!Parse(namespace_name.substr(2), namespace_index)) {
      return scada::NodeId{};
    }
  }

  const auto& identifier = scada_string.substr(p + 1);

  if (identifier.find('!') != std::string_view::npos) {
    return scada::NodeId{std::string{identifier},
                         static_cast<scada::NamespaceIndex>(namespace_index)};
  }

  scada::NumericId numeric_id = 0;
  if (!Parse(identifier, numeric_id)) {
    return scada::NodeId{std::string{identifier},
                         static_cast<scada::NamespaceIndex>(namespace_index)};
  }

  return scada::NodeId{numeric_id,
                       static_cast<scada::NamespaceIndex>(namespace_index)};
}

void RegisterModelNamespaceResolver() {
  scada::SetNamespaceNameResolver(&scada::GetNamespaceName);
}

}  // namespace scada
