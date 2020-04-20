#pragma once

#include "core/view_service.h"

#include <memory>

namespace scada {
class AddressSpace;
class Node;
}

struct ViewServiceImplContext {
  scada::AddressSpace& address_space_;
};

class ViewServiceImpl : private ViewServiceImplContext,
                        public scada::ViewService {
 public:
  explicit ViewServiceImpl(ViewServiceImplContext&& context);
  ~ViewServiceImpl();

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& descriptions,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePath(
      const scada::NodeId& starting_node_id,
      const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback) override;

 private:
  scada::BrowseResult Browse(const scada::BrowseDescription& description,
                             scada::ViewService*& async_view_service);
  scada::BrowseResult BrowseNode(const scada::Node& node,
                                 const scada::BrowseDescription& description);
  scada::BrowseResult BrowseProperty(
      const scada::Node& node,
      base::StringPiece nested_name,
      const scada::BrowseDescription& description);
};
