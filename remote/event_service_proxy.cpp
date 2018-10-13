#include "remote/event_service_proxy.h"

#include "remote/message_sender.h"
#include "remote/protocol.h"

EventServiceProxy::EventServiceProxy() {}

EventServiceProxy::~EventServiceProxy() {}

void EventServiceProxy::OnChannelOpened(MessageSender& sender) {
  sender_ = &sender;
}

void EventServiceProxy::OnChannelClosed() {
  sender_ = nullptr;
}

void EventServiceProxy::Acknowledge(int acknowledge_id,
                                    const scada::NodeId& user_node_id) {
  if (!sender_) {
    assert(false);
    return;
  }

  protocol::Request request;
  auto& acknowledge = *request.mutable_call()->mutable_acknowledge();
  acknowledge.set_acknowledge_id(acknowledge_id);

  sender_->Request(request, nullptr);
}
