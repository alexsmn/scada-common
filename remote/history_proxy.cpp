#include "remote/history_proxy.h"

#include "core/standard_node_ids.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

HistoryProxy::HistoryProxy() {}

void HistoryProxy::HistoryReadRaw(
    const scada::HistoryReadRawDetails& details,
    const scada::HistoryReadRawCallback& callback) {
  if (!sender_) {
    assert(false);
    return;
  }

  protocol::Request request;
  auto& history_read_raw = *request.mutable_history_read_raw();
  ToProto(details.node_id, *history_read_raw.mutable_node_id());
  if (!details.from.is_null())
    history_read_raw.set_from_time(details.from.ToInternalValue());
  if (!details.to.is_null())
    history_read_raw.set_to_time(details.to.ToInternalValue());
  if (details.max_count != 0)
    history_read_raw.set_max_count(details.max_count);
  if (!details.aggregation.is_null())
    ToProto(details.aggregation, *history_read_raw.mutable_aggregate_filter());
  if (details.release_continuation_point)
    history_read_raw.set_release_continuation_point(true);
  if (!details.continuation_point.empty()) {
    ToProto(details.continuation_point,
            *history_read_raw.mutable_continuation_point());
  }

  sender_->Request(
      request, [this, callback](const protocol::Response& response) {
        if (!callback)
          return;

        callback(FromProto(response.status()),
                 VectorFromProto<scada::DataValue>(
                     response.history_read_raw_result().value()),
                 FromProto<scada::ByteString>(
                     response.history_read_raw_result().continuation_point()));
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
  if (!from.is_null())
    history_read_events.set_from_time(from.ToInternalValue());
  if (!to.is_null())
    history_read_events.set_to_time(to.ToInternalValue());

  if (filter.types) {
    auto& proto_filter = *history_read_events.mutable_filter();
    if (filter.types & scada::Event::ACKED)
      proto_filter.set_acked(true);
    if (filter.types & scada::Event::UNACKED)
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
