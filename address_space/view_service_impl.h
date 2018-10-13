#pragma once

#include "address_space/node_observer.h"
#include "base/observer_list.h"
#include "core/model_change_event.h"
#include "core/view_service.h"

#include <memory>

namespace scada {
class AddressSpace;
}

struct ViewServiceImplContext {
  scada::AddressSpace& address_space_;
};

class ViewServiceImpl : private ViewServiceImplContext,
                        private scada::NodeObserver,
                        public scada::ViewService {
 public:
  explicit ViewServiceImpl(ViewServiceImplContext&& context);
  ~ViewServiceImpl();

  void NotifyModelChanged(const scada::ModelChangeEvent& event);
  void NotifySemanticChanged(const scada::NodeId& node_id,
                             const scada::NodeId& type_definion_id);

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& descriptions,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePath(
      const scada::NodeId& starting_node_id,
      const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback) override;
  virtual void Subscribe(scada::ViewEvents& events) override;
  virtual void Unsubscribe(scada::ViewEvents& events) override;

 private:
  scada::BrowseResult Browse(const scada::BrowseDescription& description,
                             scada::ViewService*& async_view_service);
  scada::BrowseResult BrowseNode(const scada::Node& node,
                                 const scada::BrowseDescription& description);
  scada::BrowseResult BrowseProperty(
      const scada::Node& node,
      base::StringPiece nested_name,
      const scada::BrowseDescription& description);

  // scada::NodeObserver
  virtual void OnNodeCreated(const scada::Node& node) override;
  virtual void OnNodeDeleted(const scada::Node& node) override;
  virtual void OnNodeModified(const scada::Node& node,
                              const scada::PropertyIds& property_ids) override;
  virtual void OnReferenceAdded(const scada::ReferenceType& reference_type,
                                const scada::Node& source,
                                const scada::Node& target) override;
  virtual void OnReferenceDeleted(const scada::ReferenceType& reference_type,
                                  const scada::Node& source,
                                  const scada::Node& target) override;

  base::ObserverList<scada::ViewEvents> events_;
};
