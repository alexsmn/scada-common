#pragma once

#include "base/logger.h"
#include "base/timer.h"
#include "base/memory/weak_ptr.h"
#include "core/view_service.h"

#include <memory>

namespace protocol {
class Reference;
class Request;
}  // namespace protocol

class MessageSender;

class ViewEventQueue {
 public:
  using Event = std::variant<scada::ModelChangeEvent, scada::NodeId>;

  void AddModelChange(const scada::ModelChangeEvent& event);
  void AddNodeSemanticChange(const scada::NodeId& node_id);

  std::vector<Event> GetEvents();

 private:
  std::vector<Event> queue_;
};

struct ViewServiceStubContext {
  const std::shared_ptr<Logger> logger_;
  boost::asio::io_context& io_context_;
  MessageSender& sender_;
  scada::ViewService& service_;
};

class ViewServiceStub final : private ViewServiceStubContext, private scada::ViewEvents {
 public:
  explicit ViewServiceStub(ViewServiceStubContext&& context);
  ~ViewServiceStub();

  void OnRequestReceived(const protocol::Request& request);

 private:
  void OnBrowse(unsigned request_id,
                const std::vector<scada::BrowseDescription>& nodes);

  void ScheduleSendEvents();
  void SendEvents();

  // scada::ViewEvents
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(const scada::NodeId& node_id) override;

  ViewEventQueue events_;

  Timer timer_;

  base::WeakPtrFactory<ViewServiceStub> weak_factory_;
};
