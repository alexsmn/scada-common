#pragma once

#include "scada/view_service.h"

#include <span>

class SyncViewService {
 public:
  virtual ~SyncViewService() = default;

  virtual std::vector<scada::BrowseResult> Browse(
      std::span<const scada::BrowseDescription> inputs) = 0;

  virtual std::vector<scada::BrowsePathResult> TranslateBrowsePaths(
      std::span<const scada::BrowsePath> inputs) = 0;
};

inline scada::BrowseResult Browse(SyncViewService& view_service,
                                  const scada::BrowseDescription& input) {
  std::span<const scada::BrowseDescription> inputs{&input, 1};
  auto results = view_service.Browse(inputs);
  assert(results.size() == 1);
  return std::move(results.front());
}
