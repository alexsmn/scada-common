#pragma once

#include "core/event_service.h"

class MessageSender;
class SubscriptionProxy;

class EventServiceProxy : public scada::EventService {
 public:
  EventServiceProxy();
  virtual ~EventServiceProxy();

  void OnChannelOpened(MessageSender& sender);
  void OnChannelClosed();

  // EventService
  virtual void Acknowledge(int acknowledge_id, const scada::NodeId& user_node_id) override;
  virtual void GenerateEvent(const scada::Event& event) override;

 private:
  MessageSender* sender_ = nullptr;

  std::unique_ptr<SubscriptionProxy> subscription_;
};
