#include "remote/history_stub.h"

#include "base/bind.h"
#include "base/logger.h"
#include "common/node_id_util.h"
#include "core/history_service.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

#include <boost/asio/io_context.hpp>

HistoryStub::HistoryStub(scada::HistoryService& service,
                         MessageSender& sender,
                         boost::asio::io_context& io_context,
                         std::shared_ptr<Logger> logger)
    : service_{service},
      sender_{sender},
      io_context_{io_context},
      logger_{std::move(logger)} {}

void HistoryStub::OnRequestReceived(const protocol::Request& request) {
  if (request.has_history_read_raw())
    OnHistoryReadRaw(request);
  if (request.has_history_read_events())
    OnHistoryReadEvents(request);
}

void HistoryStub::OnHistoryReadRaw(const protocol::Request& request) {
  auto request_id = request.request_id();
  auto& history_read_raw = request.history_read_raw();
  auto node_id = FromProto(history_read_raw.node_id());
  auto from = base::Time::FromInternalValue(history_read_raw.from());
  auto to = history_read_raw.has_to()
                ? base::Time::FromInternalValue(history_read_raw.to())
                : base::Time();

  logger_->WriteF(LogSeverity::Normal, "History read raw request %u node %s",
                  request_id, NodeIdToScadaString(node_id).c_str());

  service_.HistoryReadRaw(
      node_id, from, to, {},
      io_context_.wrap([this, weak_ptr = weak_factory_.GetWeakPtr(),
                        request_id](
                           scada::Status status,
                           std::vector<scada::DataValue> values,
                           scada::ContinuationPoint continuation_point) {
        if (!weak_ptr.get())
          return;

        logger_->WriteF(LogSeverity::Normal,
                        "History read raw request %u completed with status %s",
                        request_id, ToString(status).c_str());

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        ToProto(status, *response.mutable_status());
        if (!values.empty()) {
          ToProto(std::move(values),
                  *response.mutable_history_read_raw_result()->mutable_value());
        }
        if (!continuation_point.empty()) {
          response.mutable_history_read_raw_result()->set_continuation_point(
              std::move(continuation_point));
        }
        sender_.Send(message);
      }));
}

void HistoryStub::OnHistoryReadEvents(const protocol::Request& request) {
  auto request_id = request.request_id();
  auto& history_read_events = request.history_read_events();
  const auto node_id = FromProto(history_read_events.node_id());
  auto from = base::Time::FromInternalValue(history_read_events.from());
  auto to = history_read_events.has_to()
                ? base::Time::FromInternalValue(history_read_events.to())
                : base::Time();
  scada::EventFilter filter;
  if (history_read_events.has_filter()) {
    auto& proto_filter = history_read_events.filter();
    if (proto_filter.has_acked() && proto_filter.acked())
      filter.types |= scada::Event::ACKED;
    if (proto_filter.has_unacked() && proto_filter.unacked())
      filter.types |= scada::Event::UNACKED;
  }

  logger_->WriteF(LogSeverity::Normal, "History read events request %u node %s",
                  request_id, NodeIdToScadaString(node_id).c_str());

  service_.HistoryReadEvents(
      node_id, from, to, filter,
      io_context_.wrap([this, weak_ptr = weak_factory_.GetWeakPtr(),
                        request_id](scada::Status status,
                                    std::vector<scada::Event> events) {
        if (!weak_ptr.get())
          return;

        logger_->WriteF(
            LogSeverity::Normal,
            "History read events request %u completed with status %s",
            request_id, ToString(status).c_str());

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        ToProto(status, *response.mutable_status());
        if (!events.empty()) {
          ToProto(
              std::move(events),
              *response.mutable_history_read_events_result()->mutable_event());
        }
        sender_.Send(message);
      }));
}
