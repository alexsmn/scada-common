#include "remote/history_proxy.h"

#include "core/standard_node_ids.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

using namespace scada;

HistoryProxy::HistoryProxy() {}

void HistoryProxy::HistoryReadRaw(
    const scada::NodeId& node_id,
    base::Time from,
    base::Time to,
    const scada::HistoryReadRawCallback& callback) {
  if (!sender_) {
    assert(false);
    return;
  }

  protocol::Request request;
  auto& history_read_raw = *request.mutable_history_read_raw();
  ToProto(node_id, *history_read_raw.mutable_node_id());
  history_read_raw.set_from(from.ToInternalValue());
  if (!to.is_null())
    history_read_raw.set_to(to.ToInternalValue());

  sender_->Request(request,
                   [this, callback](const protocol::Response& response) {
                     if (!callback)
                       return;

                     callback(FromProto(response.status()),
                              VectorFromProto<scada::DataValue>(
                                  response.history_read_raw_result().value()));
                   });
}

void HistoryProxy::HistoryReadEvents(
    const scada::NodeId& node_id,
    base::Time from,
    base::Time to,
    const scada::EventFilter& filter,
    const scada::HistoryReadEventsCallback& callback) {
  if (!sender_) {
    assert(false);
    return;
  }

  protocol::Request request;
  auto& history_read_events = *request.mutable_history_read_events();
  ToProto(node_id, *history_read_events.mutable_node_id());
  history_read_events.set_from(from.ToInternalValue());
  if (!to.is_null())
    history_read_events.set_to(to.ToInternalValue());

  if (filter.types) {
    auto& proto_filter = *history_read_events.mutable_filter();
    if (filter.types & Event::ACKED)
      proto_filter.set_acked(true);
    if (filter.types & Event::UNACKED)
      proto_filter.set_unacked(true);
  }

  sender_->Request(
      request, [this, callback](const protocol::Response& response) {
        if (!callback)
          return;

        callback(FromProto(response.status()),
                 VectorFromProto<scada::Event>(
                     response.history_read_events_result().event()));
      });
}

void HistoryProxy::OnChannelOpened(MessageSender& sender) {
  sender_ = &sender;
}

void HistoryProxy::OnChannelClosed() {
  sender_ = nullptr;
}
