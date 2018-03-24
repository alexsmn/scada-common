#pragma once

#include "base/logger.h"
#include "base/memory/weak_ptr.h"
#include "core/view_service.h"

#include <memory>

namespace protocol {
class Reference;
class Request;
}  // namespace protocol

class MessageSender;

class ViewServiceStub final : private scada::ViewEvents {
 public:
  ViewServiceStub(MessageSender& sender,
                  scada::ViewService& service,
                  std::shared_ptr<Logger> logger);
  ~ViewServiceStub();

  void OnRequestReceived(const protocol::Request& request);

 private:
  void OnBrowse(unsigned request_id,
                const std::vector<scada::BrowseDescription>& nodes);

  // scada::ViewEvents
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(const scada::NodeId& node_id) override;

  MessageSender& sender_;

  scada::ViewService& service_;

  std::shared_ptr<Logger> logger_;

  base::WeakPtrFactory<ViewServiceStub> weak_factory_;
};
