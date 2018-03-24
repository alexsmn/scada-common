#include "remote/history_stub.h"

#include "base/bind.h"
#include "base/logger.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "common/node_id_util.h"
#include "core/history_service.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

HistoryStub::HistoryStub(scada::HistoryService& service,
                         MessageSender& sender,
                         std::shared_ptr<Logger> logger)
    : service_{service}, sender_{sender}, logger_{std::move(logger)} {}

void HistoryStub::OnRequestReceived(const protocol::Request& request) {
  if (!request.has_history_read())
    return;

  auto request_id = request.request_id();

  auto& history_read = request.history_read();

  scada::ReadValueId read_value_id{FromProto(history_read.node_id()),
                                   FromProto(history_read.attribute_id())};
  auto from = base::Time::FromInternalValue(history_read.from());
  auto to = history_read.has_to()
                ? base::Time::FromInternalValue(history_read.to())
                : base::Time();

  scada::Filter filter;
  if (history_read.has_event_filter()) {
    auto& event_filter = history_read.event_filter();
    if (event_filter.has_acked() && event_filter.acked())
      filter.event_filter.types |= scada::Event::ACKED;
    if (event_filter.has_unacked() && event_filter.unacked())
      filter.event_filter.types |= scada::Event::UNACKED;
  }

  logger_->WriteF(LogSeverity::Normal, "Read history %u node %s", request_id,
                  NodeIdToScadaString(read_value_id.node_id).c_str());

  auto weak_ptr = weak_factory_.GetWeakPtr();
  auto runner = base::SequencedTaskRunnerHandle::Get();
  service_.HistoryRead(
      read_value_id, from, to, filter,
      [runner, weak_ptr, request_id](scada::Status status,
                                     scada::QueryValuesResults values,
                                     scada::QueryEventsResults events) {
        runner->PostTask(
            FROM_HERE,
            base::Bind(&HistoryStub::OnHistoryReadCompleted, weak_ptr,
                       request_id, status, values, events));
      });
}

void HistoryStub::OnHistoryReadCompleted(unsigned request_id,
                                         const scada::Status& status,
                                         scada::QueryValuesResults values,
                                         scada::QueryEventsResults events) {
  logger_->WriteF(LogSeverity::Normal,
                  "Read history %u completed with result %s", request_id,
                  ToString(status).c_str());

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
}
