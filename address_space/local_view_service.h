#pragma once

#include "scada/node_id.h"
#include "scada/view_service.h"

#include <map>
#include <vector>

namespace boost::json {
class value;
}

namespace scada {

// In-memory ViewService backed by a fixed parent→children map. Answers
// `Browse` requests from the map and returns empty results for unknown
// parents. `TranslateBrowsePaths` always succeeds with an empty target list
// (mirrors the stub the screenshot generator previously installed).
//
// Intended for tests, demos, and screenshot tooling that need a SCADA back-end
// driven by static data rather than a real server.
class LocalViewService : public ViewService {
 public:
  LocalViewService();
  ~LocalViewService() override;

  void AddChildren(const NodeId& parent, std::vector<NodeId> children);

  // Populates the tree from a screenshot-style JSON document. The `tree`
  // object uses "<ns>.<id>" or bare "<id>" (implies ns=1) as keys and arrays
  // of numeric child ids (namespace 1) as values.
  void LoadFromJson(const boost::json::value& root);

  // ViewService
  void Browse(const ServiceContext& context,
              const std::vector<BrowseDescription>& inputs,
              const BrowseCallback& callback) override;
  void TranslateBrowsePaths(
      const std::vector<BrowsePath>& inputs,
      const TranslateBrowsePathsCallback& callback) override;

 private:
  std::map<NodeId, std::vector<NodeId>> children_;
};

}  // namespace scada
