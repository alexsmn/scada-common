#pragma once

#include "core/view_service.h"

#include <memory>

namespace scada {
class AddressSpace;
class Node;
}  // namespace scada

struct ViewServiceImplContext {
  scada::AddressSpace& address_space_;
};

class SyncViewServiceImpl : protected ViewServiceImplContext {
 public:
  explicit SyncViewServiceImpl(ViewServiceImplContext&& context);

  scada::BrowseResult Browse(const scada::BrowseDescription& description);

 private:
  scada::BrowseResult BrowseNode(const scada::Node& node,
                                 const scada::BrowseDescription& description);
  scada::BrowseResult BrowseProperty(
      const scada::Node& node,
      std::string_view nested_name,
      const scada::BrowseDescription& description);
};

class ViewServiceImpl : public scada::ViewService {
 public:
  explicit ViewServiceImpl(SyncViewServiceImpl& sync_service_impl);

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& descriptions,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

 private:
  SyncViewServiceImpl& sync_view_service_impl_;
};
