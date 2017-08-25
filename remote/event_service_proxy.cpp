#include "remote/event_service_proxy.h"

#include "base/strings/sys_string_conversions.h"
#include "core/monitored_item.h"
#include "remote/subscription_proxy.h"
#include "remote/protocol.h"
#include "remote/message_sender.h"

using namespace scada;

EventServiceProxy::EventServiceProxy() {
}

EventServiceProxy::~EventServiceProxy() {
}

void EventServiceProxy::OnChannelOpened(MessageSender& sender) {
  sender_ = &sender;
}

void EventServiceProxy::OnChannelClosed() {
}

void EventServiceProxy::Acknowledge(int acknowledge_id, const NodeId& user_node_id) {
  if (!sender_) {
    assert(false);
    return;
  }

  protocol::Request request;
  auto& acknowledge = *request.mutable_call()->mutable_acknowledge();
  acknowledge.set_event_id(acknowledge_id);

  sender_->Request(request, nullptr);
}

void EventServiceProxy::GenerateEvent(const Event& event) {
  assert(false);
}
