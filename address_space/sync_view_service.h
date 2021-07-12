#pragma once

#include "core/view_service.h"

class SyncViewService {
 public:
  virtual ~SyncViewService() = default;

  virtual scada::BrowseResult Browse(const scada::BrowseDescription& input) = 0;

  virtual scada::BrowsePathResult TranslateBrowsePath(
      const scada::BrowsePath& input) = 0;
};
