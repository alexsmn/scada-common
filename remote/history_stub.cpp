#include "remote/history_stub.h"

#include "base/logger.h"
#include "base/task_runner.h"
#include "core/history_service.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

using namespace scada;

HistoryStub::HistoryStub(MessageSender& sender,
                         scada::HistoryService& history_service,
                         std::shared_ptr<Logger> logger)
    : sender_(sender),
      history_service_(history_service_),
      logger_(std::move(logger)),
      weak_factory_(this) {}

void HistoryStub::OnRequestReceived(const protocol::Request& request) {
  if (!request.has_history_read())
    return;

  auto& history_read = request.history_read();

  scada::Filter filter{};
  if (history_read.has_event_filter()) {
    auto& event_filter = history_read.event_filter();
    if (event_filter.has_acked() && event_filter.acked())
      filter.event_filter.types |= scada::Event::ACKED;
    if (event_filter.has_unacked() && event_filter.unacked())
      filter.event_filter.types |= scada::Event::UNACKED;
  }

  scada::ReadValueId read_value_id{FromProto(history_read.node_id()),
                                   FromProto(history_read.attribute_id())};
  auto from = base::Time::FromInternalValue(history_read.from());
  auto to = history_read.has_to()
                ? base::Time::FromInternalValue(history_read.to())
                : base::Time();

  auto weak_ptr = weak_factory_.GetWeakPtr();
  auto request_id = request.request_id();
  history_service_.HistoryRead(
      read_value_id, from, to, filter,
      [weak_ptr, this, request_id](Status status, QueryValuesResults values,
                                   QueryEventsResults events) {
        if (!weak_ptr.get())
          return;
        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        auto& history_read_result = *response.mutable_history_read_result();
        if (values)
          ToProto(*values, *history_read_result.mutable_values());
        if (events)
          ToProto(*events, *history_read_result.mutable_events());
        ToProto(status, *response.mutable_status());
        sender_.Send(message);
      });
}

void HistoryStub::OnQueryEventsCompleted(unsigned request_id,
                                         const QueryEventsResults results) {
  protocol::Message message;
  auto& response = *message.add_responses();
  response.set_request_id(request_id);
  auto& history_read_result = *response.mutable_history_read_result();
  ToProto(StatusCode::Good, *response.mutable_status());
  if (results)
    ContainerToProto(*results, *history_read_result.mutable_events());
  sender_.Send(message);
}
