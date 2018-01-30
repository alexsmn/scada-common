#include "remote/history_proxy.h"

#include "core/standard_node_ids.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

using namespace scada;

HistoryProxy::HistoryProxy() {}

void HistoryProxy::HistoryRead(const scada::ReadValueId& read_value_id,
                               base::Time from,
                               base::Time to,
                               const scada::Filter& filter,
                               const scada::HistoryReadCallback& callback) {
  if (!sender_) {
    assert(false);
    return;
  }

  protocol::Request request;
  auto& history_read = *request.mutable_history_read();
  ToProto(read_value_id.node_id, *history_read.mutable_node_id());
  history_read.set_attribute_id(ToProto(read_value_id.attribute_id));
  history_read.set_from(from.ToInternalValue());
  if (!to.is_null())
    history_read.set_to(to.ToInternalValue());

  if (read_value_id.attribute_id == scada::AttributeId::EventNotifier) {
    auto& event_filter = *history_read.mutable_event_filter();
    if (filter.event_filter.types & Event::ACKED)
      event_filter.set_acked(true);
    if (filter.event_filter.types & Event::UNACKED)
      event_filter.set_unacked(true);
  }

  sender_->Request(
      request, [this, callback](const protocol::Response& response) {
        if (!callback)
          return;

        auto status = FromProto(response.status());

        QueryValuesResults values;
        QueryEventsResults events;
        if (response.has_history_read_result()) {
          values = std::make_shared<DataValueVector>(VectorFromProto<DataValue>(
              response.history_read_result().values()));
          events = std::make_shared<EventVector>(
              VectorFromProto<Event>(response.history_read_result().events()));
        }

        callback(status, std::move(values), std::move(events));
      });
}

void HistoryProxy::OnChannelOpened(MessageSender& sender) {
  sender_ = &sender;
}

void HistoryProxy::OnChannelClosed() {
  sender_ = nullptr;
}
