#pragma once

#include "address_space/sync_view_service.h"
#include "scada/view_service.h"

#include <memory>

namespace scada {
class AddressSpace;
class Node;
}  // namespace scada

struct ViewServiceImplContext {
  const scada::AddressSpace& address_space_;
};

class SyncViewServiceImpl : private ViewServiceImplContext,
                            public SyncViewService {
 public:
  explicit SyncViewServiceImpl(ViewServiceImplContext&& context);

  // SyncViewService
  virtual std::vector<scada::BrowseResult> Browse(
      base::span<const scada::BrowseDescription> inputs) override;
  virtual std::vector<scada::BrowsePathResult> TranslateBrowsePaths(
      base::span<const scada::BrowsePath> inputs) override;

 private:
  scada::BrowseResult BrowseOne(const scada::BrowseDescription& inputs);
  scada::BrowseResult BrowseNode(const scada::Node& node,
                                 const scada::BrowseDescription& description);
  scada::BrowseResult BrowseProperty(
      const scada::Node& node,
      std::string_view nested_name,
      const scada::BrowseDescription& description);

  scada::BrowsePathResult TranslateBrowsePath(const scada::BrowsePath& input);
};

class ViewServiceImpl : public scada::ViewService {
 public:
  explicit ViewServiceImpl(SyncViewService& sync_service);

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& descriptions,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

 private:
  SyncViewService& sync_view_service_;
};
