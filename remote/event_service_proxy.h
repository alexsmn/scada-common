#pragma once

#include "core/event_service.h"

class MessageSender;

class EventServiceProxy : public scada::EventService {
 public:
  EventServiceProxy();
  virtual ~EventServiceProxy();

  void OnChannelOpened(MessageSender& sender);
  void OnChannelClosed();

  // scada::EventService
  virtual void Acknowledge(int acknowledge_id,
                           const scada::NodeId& user_node_id) override;

 private:
  MessageSender* sender_ = nullptr;
};
