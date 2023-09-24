#include "timed_data/timed_data_impl.h"

#include "base/cancelation.h"
#include "base/debug_util.h"
#include "base/executor.h"
#include "base/format_time.h"
#include "base/interval.h"
#include "common/data_value_util.h"
#include "common/formula_util.h"
#include "events/event_set.h"
#include "events/node_event_provider.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "scada/attribute_service.h"
#include "scada/history_service.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "timed_data/timed_data_spec.h"
#include "timed_data/timed_data_util.h"

#include "base/debug_util-inl.h"

namespace {
const size_t kMaxReadCount = 10000;
}

// TimedDataImpl

TimedDataImpl::TimedDataImpl(scada::AggregateFilter aggregate_filter,
                             TimedDataContext context)
    : TimedDataContext{std::move(context)},
      aggregate_filter_{std::move(aggregate_filter)} {}

TimedDataImpl::~TimedDataImpl() {
  SetNode(nullptr);
}

void TimedDataImpl::Init(NodeRef node) {
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

  monitored_value_->Subscribe(
      static_cast<scada::DataChangeHandler>(BindCancelation(
          weak_from_this(), [this](const scada::DataValue& data_value) {
            OnChannelData(data_value);
          })));
}

void TimedDataImpl::SetNode(const NodeRef& node) {
  if (node_ == node)
    return;

  if (node_) {
    node_event_provider_.RemoveItemObserver(node_.node_id(), *this);
    node_.Unsubscribe(*this);
  }

  node_ = node;

  if (node_) {
    node_.Subscribe(*this);
    node_.Fetch(NodeFetchStatus::NodeOnly());

    node_event_provider_.AddItemObserver(node_.node_id(), *this);
    alerting_ = node_event_provider_.IsAlerting(node_.node_id());
  }
}

NodeRef TimedDataImpl::GetNode() const {
  return node_;
}

void TimedDataImpl::FetchNextGap() {
  if (querying_)
    return;

  if (!node_)
    return;

  auto gap = timed_data_view_.FindNextGap();
  if (!gap)
    return;

  assert(!IsEmptyInterval(*gap));
  assert(historical());

  querying_ = true;
  querying_range_ = *gap;

  LOG_INFO(logger_) << "Querying history"
                    << LOG_TAG("From", FormatTime(querying_range_.first))
                    << LOG_TAG("To", FormatTime(querying_range_.second));

  // Query history in the backward direction.
  scada::HistoryReadRawDetails details{node_.node_id(), querying_range_.first,
                                       querying_range_.second, kMaxReadCount,
                                       aggregate_filter_};
  // Canot use |BindCancelation| as |ScopedContinuationPoint| must always be
  // handled.
  assert(services_.history_service);
  services_.history_service->HistoryReadRaw(
      details,
      BindExecutor(executor_, [weak_ptr = weak_from_this(),
                               &history_service = *services_.history_service,
                               details](scada::HistoryReadRawResult result) {
        ScopedContinuationPoint scoped_continuation_point{
            history_service, details, std::move(result.continuation_point)};
        if (auto ptr = weak_ptr.lock()) {
          ptr->OnHistoryReadRawComplete(std::move(result.values),
                                        std::move(scoped_continuation_point));
        }
      }));
}

void TimedDataImpl::FetchMore(ScopedContinuationPoint continuation_point) {
  assert(historical());
  assert(querying_);
  assert(!continuation_point.empty());

  if (!node_) {
    LOG_INFO(logger_) << "Node was deleted";
    querying_ = false;
    return;
  }

  // Reset query if the requested range is no more interesting.
  auto gap = timed_data_view_.FindNextGap();
  if (!gap || !IntervalContains(*gap, querying_range_)) {
    LOG_INFO(logger_) << "Query canceled" << LOG_TAG("Gap", ToString(*gap))
                      << LOG_TAG("Range", ToString(querying_range_));
    continuation_point.reset();
    querying_ = false;
    FetchNextGap();
    return;
  }

  LOG_INFO(logger_) << "Continue querying history"
                    << LOG_TAG("Range", ToString(querying_range_));

  // Query history in the backward direction.
  scada::HistoryReadRawDetails details{node_.node_id(),
                                       querying_range_.second,
                                       querying_range_.first,
                                       kMaxReadCount,
                                       aggregate_filter_,
                                       false,
                                       continuation_point.release()};
  // Canot use |BindCancelation| as |ScopedContinuationPoint| must always be
  // handled.
  assert(services_.history_service);
  services_.history_service->HistoryReadRaw(
      details,
      BindExecutor(executor_, [weak_ptr = weak_from_this(),
                               &history_service = *services_.history_service,
                               details](scada::HistoryReadRawResult result) {
        ScopedContinuationPoint scoped_continuation_point{
            history_service, details, std::move(result.continuation_point)};
        if (auto ptr = weak_ptr.lock()) {
          ptr->OnHistoryReadRawComplete(std::move(result.values),
                                        std::move(scoped_continuation_point));
        }
      }));
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

  for (auto& o : observers_) {
    o.OnTimedDataNodeModified();
  }
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
                                        const EventSet& events) {
  assert(node_id == node_.node_id());

  alerting_ = !events.empty();
  NotifyEventsChanged();
}

void TimedDataImpl::OnHistoryReadRawComplete(
    std::vector<scada::DataValue> values,
    ScopedContinuationPoint continuation_point) {
  assert(historical());
  assert(querying_);
  assert(IsTimeSorted(values));
  assert(std::none_of(
      values.begin(), values.end(),
      [](const scada::DataValue& v) { return v.server_timestamp.is_null(); }));

  auto ref = shared_from_this();

  timed_data_view_.ReplaceRange(values);

  scada::DateTime ready_to;
  if (continuation_point.empty())
    ready_to = querying_range_.second;
  else if (!values.empty())
    ready_to = values.back().source_timestamp;

  if (!ready_to.is_null()) {
    LOG_INFO(logger_) << "Query result" << LOG_TAG("ValueCount", values.size())
                      << LOG_TAG("ReadFrom", FormatTime(querying_range_.first))
                      << LOG_TAG("ReadTo", FormatTime(ready_to));

    timed_data_view_.AddReadyRange({querying_range_.first, ready_to});

    assert(ready_to >= querying_range_.first);
    assert(ready_to <= querying_range_.second);
    querying_range_.first = ready_to;
  }

  // Query next fragment of data if |from_| changed from moment of last query.
  // Note that |from_| is allowed to be increased.
  if (continuation_point.empty()) {
    LOG_INFO(logger_) << "Query completed";
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
    if (UpdateCurrent(data_value)) {
      NotifyPropertyChanged(PropertySet(PROPERTY_CURRENT));
    }
  } else {
    if (timed_data_view_.InsertOrUpdate(data_value)) {
      timed_data_view_.NotifyTimedDataCorrection(1, &data_value);
    }
  }
}

const EventSet* TimedDataImpl::GetEvents() const {
  return node_ ? node_event_provider_.GetItemUnackedEvents(node_.node_id())
               : nullptr;
}

void TimedDataImpl::Acknowledge() {
  if (node_)
    node_event_provider_.AcknowledgeItemEvents(node_.node_id());
}
