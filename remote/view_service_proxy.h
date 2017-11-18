#pragma once

#include <memory>

#include "base/observer_list.h"
#include "core/view_service.h"

namespace protocol {
class BrowseResult;
class Notification;
}

class Logger;
class MessageSender;

class ViewServiceProxy : public scada::ViewService {
 public:
  explicit ViewServiceProxy(std::shared_ptr<Logger> logger);

  void OnNotification(const protocol::Notification& notification);

  void OnChannelOpened(MessageSender& sender);
  void OnChannelClosed();

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& nodes, const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePath(const scada::NodeId& starting_node_id, const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback) override;
  virtual void Subscribe(scada::ViewEvents& events) override;
  virtual void Unsubscribe(scada::ViewEvents& events) override;

 private:
  const std::shared_ptr<Logger> logger_;

  MessageSender* sender_ = nullptr;

  base::ObserverList<scada::ViewEvents> events_;
};
