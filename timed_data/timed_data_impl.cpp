#include "timed_data/timed_data_impl.h"

#include "base/bind.h"
#include "base/format_time.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "common/data_value_util.h"
#include "common/event_manager.h"
#include "common/formula_util.h"
#include "common/interval_util.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "core/attribute_service.h"
#include "core/debug_util.h"
#include "core/event_service.h"
#include "core/history_service.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"
#include "timed_data/timed_data_spec.h"
#include "timed_data/timed_data_util.h"

namespace rt {

namespace {

const size_t kMaxReadCount = 10000;

std::optional<DataValues::iterator> FindInsertPosition(DataValues& values,
                                                       base::Time from,
                                                       base::Time to) {
  auto i = LowerBound(values, from);
  auto j = LowerBound(values, to);
  if (i != j)
    return std::nullopt;
  if (i != values.end() && i->source_timestamp == from)
    return std::nullopt;
  if (j != values.end() && j->source_timestamp == to)
    return std::nullopt;
  return i;
}

}  // namespace

// TimedDataImpl

TimedDataImpl::TimedDataImpl(NodeRef node,
                             scada::AggregateFilter aggregate_filter,
                             TimedDataContext context,
                             std::shared_ptr<const Logger> logger)
    : TimedDataContext{std::move(context)},
      BaseTimedData{std::move(logger)},
      aggregate_filter_{std::move(aggregate_filter)} {
  assert(node);
  SetNode(std::move(node));

  scada::MonitoringParameters params;
  if (!aggregate_filter_.is_null())
    params.filter = aggregate_filter_;

  monitored_value_ =
      node_.CreateMonitoredItem(scada::AttributeId::Value, params);
  if (!monitored_value_) {
    Delete();
    return;
  }

  monitored_value_->set_data_change_handler(
      [this](const scada::DataValue& data_value) {
        OnChannelData(data_value);
      });
  monitored_value_->Subscribe();
}

TimedDataImpl::~TimedDataImpl() {
  SetNode(nullptr);
}

void TimedDataImpl::SetNode(const NodeRef& node) {
  if (node_ == node)
    return;

  if (node_) {
    event_manager_.RemoveItemObserver(node_.node_id(), *this);
    node_.Unsubscribe(*this);
  }

  node_ = node;

  if (node_) {
    node_.Subscribe(*this);
    event_manager_.AddItemObserver(node_.node_id(), *this);

    alerting_ = event_manager_.IsAlerting(node_.node_id());
  }
}

NodeRef TimedDataImpl::GetNode() const {
  return node_;
}

void TimedDataImpl::Write(double value,
                          const scada::NodeId& user_id,
                          const scada::WriteFlags& flags,
                          const StatusCallback& callback) const {
  node_.Write(scada::AttributeId::Value, value, flags, user_id, callback);
}

void TimedDataImpl::Call(const scada::NodeId& method_id,
                         const std::vector<scada::Variant>& arguments,
                         const scada::NodeId& user_id,
                         const StatusCallback& callback) const {
  node_.Call(method_id, arguments, user_id, callback);
}

void TimedDataImpl::FetchNextGap() {
  if (querying_)
    return;

  if (!node_)
    return;

  auto gap = FindNextGap();
  if (!gap)
    return;

  assert(!IsEmptyInterval(*gap));
  querying_ = true;
  querying_range_ = *gap;

  logger_->WriteF(LogSeverity::Normal, "Querying history from %s to %s",
                  FormatTime(querying_range_.first).c_str(),
                  FormatTime(querying_range_.second).c_str());

  auto message_loop = base::ThreadTaskRunnerHandle::Get();
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();

  // Query history in the backward direction.
  scada::HistoryReadRawDetails details{node_.node_id(), querying_range_.first,
                                       querying_range_.second, kMaxReadCount,
                                       aggregate_filter_};
  history_service_.HistoryReadRaw(
      details,
      [message_loop, weak_ptr, &history_service = history_service_, details](
          scada::Status status, std::vector<scada::DataValue> values,
          scada::ByteString continuation_point) {
        ScopedContinuationPoint scoped_continuation_point{
            history_service, details, std::move(continuation_point)};
        message_loop->PostTask(
            FROM_HERE,
            base::Bind(&TimedDataImpl::OnHistoryReadRawComplete, weak_ptr,
                       base::Passed(std::move(values)),
                       base::Passed(std::move(scoped_continuation_point))));
      });
}

void TimedDataImpl::FetchMore(ScopedContinuationPoint continuation_point) {
  assert(querying_);
  assert(!continuation_point.empty());

  if (!node_) {
    logger_->Write(LogSeverity::Normal, "Node was deleted");
    querying_ = false;
    return;
  }

  // Reset query if the requested range is no more interesting.
  auto gap = FindNextGap();
  if (!gap || !IntervalContains(*gap, querying_range_)) {
    logger_->WriteF(LogSeverity::Normal,
                    "Query canceled. Gap is %s, querying range is %s",
                    ToString(*gap).c_str(), ToString(querying_range_).c_str());
    continuation_point.reset();
    querying_ = false;
    FetchNextGap();
    return;
  }

  logger_->WriteF(LogSeverity::Normal, "Continue querying history %s",
                  ToString(querying_range_).c_str());

  auto message_loop = base::ThreadTaskRunnerHandle::Get();
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();

  // Query history in the backward direction.
  scada::HistoryReadRawDetails details{node_.node_id(),
                                       querying_range_.second,
                                       querying_range_.first,
                                       kMaxReadCount,
                                       aggregate_filter_,
                                       false,
                                       continuation_point.release()};
  history_service_.HistoryReadRaw(
      details,
      [message_loop, weak_ptr, &history_service = history_service_, details](
          scada::Status status, std::vector<scada::DataValue> values,
          scada::ByteString continuation_point) {
        ScopedContinuationPoint scoped_continuation_point{
            history_service, details, std::move(continuation_point)};
        message_loop->PostTask(
            FROM_HERE,
            base::Bind(&TimedDataImpl::OnHistoryReadRawComplete, weak_ptr,
                       base::Passed(std::move(values)),
                       base::Passed(std::move(scoped_continuation_point))));
      });
}

void TimedDataImpl::OnRangesChanged() {
  FetchNextGap();
}

std::string TimedDataImpl::GetFormula(bool aliases) const {
  return MakeNodeIdFormula(node_.node_id());
}

scada::LocalizedText TimedDataImpl::GetTitle() const {
  return node_.display_name();
}

void TimedDataImpl::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  assert(node_id == node_.node_id());

  NotifyPropertyChanged(PropertySet(PROPERTY_TITLE | PROPERTY_CURRENT));

  for (auto& o : observers_)
    o.OnTimedDataNodeModified();
}

void TimedDataImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    SetNode(nullptr);
    Delete();

  } else {
    NotifyPropertyChanged(PropertySet(PROPERTY_TITLE | PROPERTY_CURRENT));

    for (auto& o : observers_)
      o.OnTimedDataNodeModified();
  }
}

void TimedDataImpl::OnItemEventsChanged(const scada::NodeId& node_id,
                                        const events::EventSet& events) {
  assert(node_id == node_.node_id());

  alerting_ = !events.empty();
  NotifyEventsChanged();
}

void TimedDataImpl::OnHistoryReadRawComplete(
    std::vector<scada::DataValue> values,
    ScopedContinuationPoint continuation_point) {
  assert(querying_);
  assert(IsTimeSorted(values));
  assert(std::none_of(
      values.begin(), values.end(),
      [](const scada::DataValue& v) { return v.server_timestamp.is_null(); }));

  auto ref = shared_from_this();

  // Merge requested data with existing realtime data by collection time.
  if (!values.empty()) {
    // Optimization: if all new values relate to the same position in history,
    // not overlaping other values, then insert them as whole.
    if (auto i = FindInsertPosition(values_, values.front().source_timestamp,
                                    values.back().source_timestamp)) {
      values_.insert(*i, std::make_move_iterator(values.begin()),
                     std::make_move_iterator(values.end()));
    } else {
      ReplaceSubrange(values_, {values.data(), values.size()},
                      DataValueTimeLess{});
    }
  }

  assert(IsTimeSorted(values_));

  scada::DateTime ready_to;
  if (continuation_point.empty())
    ready_to = querying_range_.second;
  else if (!values.empty())
    ready_to = values.back().source_timestamp;

  if (!ready_to.is_null()) {
    logger_->WriteF(LogSeverity::Normal,
                    "Query result %Iu values. Ready from %s to %s",
                    values.size(), FormatTime(querying_range_.first).c_str(),
                    ToString(ready_to).c_str());

    SetReady({querying_range_.first, ready_to});

    assert(ready_to >= querying_range_.first);
    assert(ready_to <= querying_range_.second);
    querying_range_.first = ready_to;
  }

  // Query next fragment of data if |from_| changed from moment of last query.
  // Note that |from_| is allowed to be increased.
  if (continuation_point.empty()) {
    logger_->Write(LogSeverity::Normal, "Query completed");
    querying_ = false;
    FetchNextGap();

  } else {
    FetchMore(std::move(continuation_point));
  }
}

void TimedDataImpl::OnChannelData(const scada::DataValue& data_value) {
  if (data_value.qualifier.failed()) {
    monitored_value_.reset();
    Delete();
    return;
  }

  if (IsUpdate(current_, data_value)) {
    if (UpdateCurrent(data_value))
      NotifyPropertyChanged(PropertySet(PROPERTY_CURRENT));
  } else {
    if (UpdateHistory(data_value))
      NotifyTimedDataCorrection(1, &data_value);
  }
}

const events::EventSet* TimedDataImpl::GetEvents() const {
  return node_ ? event_manager_.GetItemUnackedEvents(node_.node_id()) : nullptr;
}

void TimedDataImpl::Acknowledge() {
  if (node_)
    event_manager_.AcknowledgeItemEvents(node_.node_id());
}

}  // namespace rt
