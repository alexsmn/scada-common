#include "address_space/local_view_service.h"

#include "scada/service_context.h"
#include "scada/standard_node_ids.h"
#include "scada/status.h"

#include <boost/json.hpp>

#include <string>

namespace scada {

namespace {

// Parses "ns.id" (e.g. "0.84") or a bare "id" (implies ns=1) into a NodeId.
NodeId ParseJsonNodeId(std::string_view s) {
  NumericId id = 0;
  NamespaceIndex ns = 1;
  if (auto dot = s.find('.'); dot != std::string_view::npos) {
    ns = static_cast<NamespaceIndex>(std::stoi(std::string(s.substr(0, dot))));
    id = static_cast<NumericId>(std::stoi(std::string(s.substr(dot + 1))));
  } else {
    id = static_cast<NumericId>(std::stoi(std::string(s)));
  }
  return NodeId{id, ns};
}

NodeId ParseJsonChildNodeId(const boost::json::value& child) {
  if (child.is_string())
    return ParseJsonNodeId(std::string_view(child.as_string()));
  return NodeId{static_cast<NumericId>(child.as_int64()), 1};
}

}  // namespace

LocalViewService::LocalViewService() = default;
LocalViewService::~LocalViewService() = default;

void LocalViewService::AddChildren(const NodeId& parent,
                                   std::vector<NodeId> children) {
  children_[parent] = std::move(children);
}

void LocalViewService::LoadFromJson(const boost::json::value& root) {
  for (const auto& [key, val] : root.at("tree").as_object()) {
    auto parent = ParseJsonNodeId(std::string_view(key));

    std::vector<NodeId> children;
    for (const auto& child : val.as_array())
      children.push_back(ParseJsonChildNodeId(child));
    AddChildren(parent, std::move(children));
  }
}

void LocalViewService::Browse(const ServiceContext& /*context*/,
                              const std::vector<BrowseDescription>& inputs,
                              const BrowseCallback& callback) {
  const NodeId organizes{id::Organizes, 0};

  std::vector<BrowseResult> results;
  results.reserve(inputs.size());

  for (const auto& desc : inputs) {
    BrowseResult result;
    result.status_code = StatusCode::Good;
    auto it = children_.find(desc.node_id);
    if (it != children_.end()) {
      for (const auto& child : it->second) {
        result.references.push_back(
            {.reference_type_id = organizes, .forward = true, .node_id = child});
      }
    }
    results.push_back(std::move(result));
  }

  callback(Status{StatusCode::Good}, std::move(results));
}

void LocalViewService::TranslateBrowsePaths(
    const std::vector<BrowsePath>& /*inputs*/,
    const TranslateBrowsePathsCallback& callback) {
  callback(Status{StatusCode::Good}, std::vector<BrowsePathResult>{});
}

}  // namespace scada
