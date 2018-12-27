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

HistoryStub::~HistoryStub() {
  // Release continuation points.
  if (!continuation_points_.empty()) {
    const scada::HistoryReadRawCallback callback =
        [](scada::Status status, std::vector<scada::DataValue> values,
           scada::ByteString continuation_point) {
          assert(values.empty());
          assert(continuation_point.empty());
        };

    for (auto& [continuation_point, details] : continuation_points_) {
      details.release_continuation_point = true;
      details.continuation_point = std::move(continuation_point);
      service_.HistoryReadRaw(details, callback);
    }
  }
}

void HistoryStub::OnRequestReceived(const protocol::Request& request) {
  if (request.has_history_read_raw())
    OnHistoryReadRaw(request);
  if (request.has_history_read_events())
    OnHistoryReadEvents(request);
}

void HistoryStub::OnHistoryReadRaw(const protocol::Request& request) {
  auto request_id = request.request_id();
  auto& history_read_raw = request.history_read_raw();

  auto continuation_point =
      FromProto<scada::ByteString>(history_read_raw.continuation_point());

  scada::HistoryReadRawDetails details;

  bool ignore_params = false;
  if (!continuation_point.empty()) {
    auto i = continuation_points_.find(continuation_point);
    if (i != continuation_points_.end()) {
      details = std::move(i->second);
      continuation_points_.erase(i);
      ignore_params = true;
    }
  }

  details.release_continuation_point =
      history_read_raw.release_continuation_point();
  details.continuation_point = std::move(continuation_point);

  if (!ignore_params) {
    details.node_id = FromProto(history_read_raw.node_id());
    details.from =
        history_read_raw.has_from_time()
            ? base::Time::FromInternalValue(history_read_raw.from_time())
            : base::Time();
    details.to = history_read_raw.has_to_time()
                     ? base::Time::FromInternalValue(history_read_raw.to_time())
                     : base::Time();
    details.max_count = history_read_raw.max_count();
    details.aggregation = history_read_raw.has_aggregate_filter()
                              ? FromProto(history_read_raw.aggregate_filter())
                              : scada::AggregateFilter{};
  }

  logger_->WriteF(LogSeverity::Normal, "History read raw request %u node %s",
                  request_id, NodeIdToScadaString(details.node_id).c_str());

  service_.HistoryReadRaw(
      details,
      io_context_.wrap([this, weak_ptr = weak_factory_.GetWeakPtr(), request_id,
                        details](scada::Status status,
                                 std::vector<scada::DataValue> values,
                                 scada::ByteString continuation_point) {
        if (!weak_ptr.get())
          return;

        logger_->WriteF(LogSeverity::Normal,
                        "History read raw request %u completed with status %s",
                        request_id, ToString(status).c_str());

        if (!continuation_point.empty())
          continuation_points_.emplace(continuation_point, details);

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        ToProto(status, *response.mutable_status());
        if (!values.empty()) {
          ToProto(std::move(values),
                  *response.mutable_history_read_raw_result()->mutable_value());
        }
        if (!continuation_point.empty()) {
          ToProto(std::move(continuation_point),
                  *response.mutable_history_read_raw_result()
                       ->mutable_continuation_point());
        }
        sender_.Send(message);
      }));
}

void HistoryStub::OnHistoryReadEvents(const protocol::Request& request) {
  auto request_id = request.request_id();
  auto& history_read_events = request.history_read_events();
  const auto node_id = FromProto(history_read_events.node_id());
  auto from =
      history_read_events.has_from_time()
          ? base::Time::FromInternalValue(history_read_events.from_time())
          : base::Time();
  auto to = history_read_events.has_to_time()
                ? base::Time::FromInternalValue(history_read_events.to_time())
                : base::Time();
  scada::EventFilter filter;
  if (history_read_events.has_filter()) {
    auto& proto_filter = history_read_events.filter();
    if (proto_filter.acked())
      filter.types |= scada::Event::ACKED;
    if (proto_filter.unacked())
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
