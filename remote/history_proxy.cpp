#include "remote/history_proxy.h"

#include "core/standard_node_ids.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

using namespace scada;

HistoryProxy::HistoryProxy()
    : sender_(nullptr) {
}

void HistoryProxy::HistoryRead(const ReadValueId& read_value_id, base::Time from, base::Time to,
                               const scada::Filter& filter, const HistoryReadCallback& callback) {
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

  sender_->Request(request,
      [this, callback](const protocol::Response& response) {
        if (!callback)
          return;
        QueryValuesResults values;
        QueryEventsResults events;
        if (response.has_history_read_result()) {
          auto& m = response.history_read_result();
          values = std::make_shared<DataValueVector>(VectorFromProto<DataValue>(m.values()));
          events = std::make_shared<EventVector>(VectorFromProto<Event>(m.events()));
        }
        callback(FromProto(response.status(0)), std::move(values), std::move(events));
      });
}

void HistoryProxy::OnChannelOpened(MessageSender& sender) {
  sender_ = &sender;
}

void HistoryProxy::OnChannelClosed() {
  sender_ = nullptr;
}
