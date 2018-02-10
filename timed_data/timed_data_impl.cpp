#include "timed_data/timed_data_impl.h"

#include "base/bind.h"
#include "base/format_time.h"
#include "base/location.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/utils.h"
#include "common/event_manager.h"
#include "common/formula_util.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "core/attribute_service.h"
#include "core/event_service.h"
#include "core/history_service.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"
#include "timed_data/timed_data_spec.h"

namespace rt {

TimedDataImpl::TimedDataImpl(const NodeRef& node,
                             const TimedDataContext& context,
                             std::shared_ptr<const Logger> parent_logger)
    : TimedDataContext{context}, querying_(false), weak_ptr_factory_(this) {
  assert(node);
  SetNode(node);

  logger_.set_parent(std::move(parent_logger));
  logger_.set_prefix(NodeIdToScadaString(node_.id()));

  monitored_value_ = monitored_item_service_.CreateMonitoredItem(
      {node_.id(), scada::AttributeId::Value});
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
    event_manager_.RemoveItemObserver(node_.id(), *this);
    node_.Unsubscribe(*this);
  }

  node_ = node;

  if (node_) {
    node_.Subscribe(*this);
    event_manager_.AddItemObserver(node_.id(), *this);

    alerting_ = event_manager_.IsAlerting(node_.id());
  }
}

NodeRef TimedDataImpl::GetNode() const {
  return node_;
}

void TimedDataImpl::Write(double value,
                          const scada::WriteFlags& flags,
                          const StatusCallback& callback) const {
  auto node = GetNode();
  if (!node) {
    if (callback)
      callback(scada::StatusCode::Bad_WrongNodeId);
    return;
  }

  attribute_service_.Write(node.id(), value, scada::NodeId(), flags, callback);
}

void TimedDataImpl::Call(const scada::NodeId& method_id,
                         const std::vector<scada::Variant>& arguments,
                         const StatusCallback& callback) const {
  auto node = GetNode();
  if (!node) {
    if (callback)
      callback(scada::StatusCode::Bad_WrongNodeId);
    return;
  }

  method_service_.Call(node.id(), method_id, arguments, callback);
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

  logger_.WriteF(LogSeverity::Normal, "Querying history from {%s} to {%s}",
                 FormatTime(from_).c_str(),
                 !to.is_null() ? FormatTime(to).c_str() : "Current");

  querying_ = true;
  auto message_loop = base::ThreadTaskRunnerHandle::Get();
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  auto range = std::make_pair(from_, to);

  if (!node_) {
    OnQueryValuesComplete(range.first, range.second,
                          scada::QueryValuesResults());
    return;
  }

  history_service_.HistoryRead(
      {node_.id(), scada::AttributeId::Value}, from_, to, {},
      [message_loop, weak_ptr, range](const scada::Status& status,
                                      scada::QueryValuesResults values,
                                      scada::QueryEventsResults events) {
        message_loop->PostTask(
            FROM_HERE, base::Bind(&TimedDataImpl::OnQueryValuesComplete,
                                  weak_ptr, range.first, range.second, values));
      });
}

void TimedDataImpl::OnFromChanged() {
  assert(historical());

  if (!querying_)
    QueryValues();
}

std::string TimedDataImpl::GetFormula(bool aliases) const {
  return MakeNodeIdFormula(node_.id());
}

scada::LocalizedText TimedDataImpl::GetTitle() const {
  return node_.display_name();
}

void TimedDataImpl::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  assert(node_id == node_.id());

  NotifyPropertyChanged(PropertySet(PROPERTY_TITLE | PROPERTY_CURRENT));

  for (auto& o : observers_)
    o.OnTimedDataNodeModified();
}

void TimedDataImpl::OnModelChange(const ModelChangeEvent& event) {
  if (event.verb & ModelChangeEvent::NodeDeleted) {
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
  assert(node_id == node_.id());

  alerting_ = !events.empty();
  NotifyEventsChanged();
}

void TimedDataImpl::OnQueryValuesComplete(base::Time queried_from,
                                          base::Time queried_to,
                                          scada::QueryValuesResults results) {
  assert(querying_);
  assert(queried_from < ready_from_);
  assert(queried_from >= from_);
  assert((!queried_to.is_null() && ready_from_ == queried_to) ||
         (queried_to.is_null() && ready_from_ == kTimedDataCurrentOnly));

  querying_ = false;

  // Merge requested data with existing realtime data by collection time.
  if (results) {
    for (auto& new_entry : *results)
      UpdateMap(new_entry);
  }

  base::Time new_ready_from = queried_from;
  // Returned data array includes left time bound.
  if (results && !results->empty() &&
      results->front().source_timestamp < new_ready_from)
    new_ready_from = results->front().source_timestamp;

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
    if (UpdateMap(data_value))
      NotifyTimedDataCorrection(1, &data_value);
  }
}

const events::EventSet* TimedDataImpl::GetEvents() const {
  if (auto node = GetNode())
    return event_manager_.GetItemUnackedEvents(node.id());
  return nullptr;
}

void TimedDataImpl::Acknowledge() {
  if (auto node = GetNode())
    event_manager_.AcknowledgeItemEvents(node.id());
}

}  // namespace rt
