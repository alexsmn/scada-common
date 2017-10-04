#include "remote/subscription_stub.h"

#include "base/strings/sys_string_conversions.h"
#include "core/monitored_item.h"
#include "core/monitored_item_service.h"
#include "core/event_service.h"
#include "remote/subscription.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

SubscriptionStub::SubscriptionStub(MessageSender& sender,
                                   scada::MonitoredItemService& realtime_service,
                                   int subscription_id,
                                   const SubscriptionParams& params)
    : sender_(sender),
      monitored_item_service_(realtime_service),
      subscription_id_(subscription_id),
      next_monitored_item_id_(1) {
}

SubscriptionStub::~SubscriptionStub() {
}

void SubscriptionStub::OnCreateMonitoredItem(int request_id, const scada::NodeId& node_id,
                                             scada::AttributeId attribute_id) {
  auto channel = monitored_item_service_.CreateMonitoredItem(node_id, attribute_id);
  if (!channel) {
    scada::DataValue data_value;
    data_value.qualifier.set_failed(true);

    protocol::Message message;
    auto& response = *message.add_responses();
    response.set_request_id(request_id);
    auto& create_monitored_item_result = *response.mutable_create_monitored_item_result();
    ToProto(scada::StatusCode::Bad_WrongNodeId, *response.add_status());
    sender_.Send(message);
    return;
  }

  auto monitored_item_id = next_monitored_item_id_++;
  assert(channels_.find(monitored_item_id) == channels_.end());

  auto channel_ptr = channel.get();
  channels_[monitored_item_id] = std::move(channel);

  {
    protocol::Message message;
    auto& response = *message.add_responses();
    response.set_request_id(request_id);
    auto& create_monitored_item_result = *response.mutable_create_monitored_item_result();
    ToProto(scada::StatusCode::Good, *response.add_status());
    create_monitored_item_result.set_monitored_item_id(monitored_item_id);
    sender_.Send(message);
  }

  if (attribute_id == OpcUa_Attributes_Value) {
    channel_ptr->set_data_change_handler(
        [this, monitored_item_id](const scada::DataValue& data_value) {
          OnDataChange(monitored_item_id, data_value);
        });
  }

  if (attribute_id == OpcUa_Attributes_EventNotifier) {
    channel_ptr->set_event_handler(
        [this, monitored_item_id](const scada::Status& status, const scada::Event& event) {
          OnEvent(monitored_item_id, status, event);
        });
  }

  channel_ptr->Subscribe();
}

void SubscriptionStub::OnDeleteMonitoredItem(int request_id, int monitored_item_id) {
  channels_.erase(monitored_item_id);

  protocol::Message message;
  auto& response = *message.add_responses();
  response.set_request_id(request_id);
  ToProto(scada::StatusCode::Good, *response.add_status());
  sender_.Send(message);
}

void SubscriptionStub::OnDataChange(MonitoredItemId monitored_item_id, const scada::DataValue& data_value) {
  auto i = channels_.find(monitored_item_id);
  if (i == channels_.end())
    return;

  if (data_value.qualifier.failed())
    channels_.erase(i);

  protocol::Message message;
  auto& notification = *message.add_notifications();
  notification.set_subscription_id(subscription_id_);
  auto& data_change = *notification.add_data_changes();
  data_change.set_monitored_item_id(monitored_item_id);
  ToProto(data_value, *data_change.mutable_data_value());
  sender_.Send(message);
}

void SubscriptionStub::OnEvent(MonitoredItemId monitored_item_id, const scada::Status& status, const scada::Event& event) {
  auto i = channels_.find(monitored_item_id);
  if (i == channels_.end())
    return;

  // TODO: Handle |status|.

  protocol::Message message;
  auto& notification = *message.add_notifications();
  notification.set_subscription_id(subscription_id_);
  auto& event_message = *notification.add_events();
  event_message.set_monitored_item_id(monitored_item_id);
  ToProto(event, event_message);
  sender_.Send(message);
}
