#include "timed_data/timed_data_impl.h"

#include "base/bind.h"
#include "base/format_time.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "common/event_manager.h"
#include "common/formula_util.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "core/attribute_service.h"
#include "core/event_service.h"
#include "core/history_service.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"
#include "timed_data/timed_data_spec.h"

namespace rt {

namespace {

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

TimedDataImpl::TimedDataImpl(NodeRef node,
                             scada::AggregateFilter aggregate_filter,
                             TimedDataContext context,
                             std::shared_ptr<const Logger> logger)
    : TimedDataContext{std::move(context)},
      logger_{std::move(logger)},
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

void TimedDataImpl::QueryValues() {
  assert(historical());

  if (from_ >= ready_from_) {
    assert(from_ == ready_from_);
    return;
  }

  // Convert upper time range bound to query specific value:
  // |kTimedDataCurrentOnly| must be replaced on Time() in such case.
  base::Time to = ready_from_;
  assert(!to.is_null());
  if (to == kTimedDataCurrentOnly)
    to = base::Time();

  logger_->WriteF(LogSeverity::Normal, "Querying history from %s to %s",
                  FormatTime(from_).c_str(),
                  !to.is_null() ? FormatTime(to).c_str() : "Current");

  querying_ = true;
  auto message_loop = base::ThreadTaskRunnerHandle::Get();
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  auto range = std::make_pair(from_, to);

  if (!node_) {
    OnHistoryReadRawComplete(range.first, range.second, {});
    return;
  }

  history_service_.HistoryReadRaw(
      node_.node_id(), from_, to, aggregate_filter_,
      [message_loop, weak_ptr, range](scada::Status&& status,
                                      std::vector<scada::DataValue>&& values) {
        message_loop->PostTask(
            FROM_HERE, base::Bind(&TimedDataImpl::OnHistoryReadRawComplete,
                                  weak_ptr, range.first, range.second,
                                  base::Passed(std::move(values))));
      });
}

void TimedDataImpl::OnFromChanged() {
  assert(historical());

  if (!querying_)
    QueryValues();
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
    base::Time queried_from,
    base::Time queried_to,
    std::vector<scada::DataValue>&& values) {
  assert(querying_);
  assert(queried_from < ready_from_);
  assert(queried_from >= from_);
  assert((!queried_to.is_null() && ready_from_ == queried_to) ||
         (queried_to.is_null() && ready_from_ == kTimedDataCurrentOnly));
  assert(
      std::is_sorted(values.begin(), values.end(),
                     [](const scada::DataValue& a, const scada::DataValue& b) {
                       return a.source_timestamp < b.source_timestamp;
                     }));
  assert(std::none_of(
      values.begin(), values.end(),
      [](const scada::DataValue& v) { return v.server_timestamp.is_null(); }));

  querying_ = false;

  // Merge requested data with existing realtime data by collection time.
  if (!values.empty()) {
    // Optimization: if all new values relate to the same position in history,
    // not overlaping other values, then insert them as whole.
    if (auto i = FindInsertPosition(values_, values.front().source_timestamp,
                                    values.back().source_timestamp)) {
      values_.insert(*i, std::make_move_iterator(values.begin()),
                     std::make_move_iterator(values.end()));
    } else {
      for (auto& value : values)
        UpdateHistory(value);
    }
  }

  base::Time new_ready_from = queried_from;
  // Returned data array includes left time bound.
  if (!values.empty() && values.front().source_timestamp < new_ready_from)
    new_ready_from = values.front().source_timestamp;

  logger_->WriteF(LogSeverity::Normal,
                  "Query result %Iu values from %s to %s. Ready from %s",
                  values.size(), FormatTime(queried_from).c_str(),
                  FormatTime(queried_to).c_str(),
                  FormatTime(new_ready_from).c_str());

  UpdateReadyFrom(new_ready_from);

  // Query next fragment of data if |from_| changed from moment of last query.
  QueryValues();

  // This probably may cause deletion.
  NotifyDataReady();
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
