#pragma once

#include "base/memory/weak_ptr.h"
#include "base/logger.h"
#include "core/configuration_types.h"
#include "core/view_service.h"

#include <memory>

namespace protocol {
class Reference;
class Request;
}

class MessageSender;

class ViewServiceStub : private scada::ViewEvents {
 public:
  ViewServiceStub(MessageSender& sender,
                  scada::ViewService& service,
                  std::shared_ptr<Logger> logger);
  ~ViewServiceStub();

  void OnRequestReceived(const protocol::Request& request);

 private:
  void OnBrowse(unsigned request_id, const std::vector<scada::BrowseDescription>& nodes);

  // scada::ViewEvents
  virtual void OnNodeAdded(const scada::NodeId& node_id) override;
  virtual void OnNodeDeleted(const scada::NodeId& node_id) override;
  virtual void OnReferenceAdded(const scada::BrowseReference& reference) override;
  virtual void OnReferenceDeleted(const scada::BrowseReference& reference) override;
  virtual void OnNodeModified(const scada::NodeId& node_id, const scada::PropertyIds& property_ids) override;

  MessageSender& sender_;

  scada::ViewService& service_;

  std::shared_ptr<Logger> logger_;

  base::WeakPtrFactory<ViewServiceStub> weak_factory_;
};
