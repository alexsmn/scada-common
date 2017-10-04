#include "timed_data_impl.h"

#include "base/format_time.h"
#include "base/strings/sys_string_conversions.h"
#include "common/formula_util.h"
#include "common/event_manager.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"
#include "common/scada_node_ids.h"
#include "timed_data/timed_data_spec.h"
#include "core/attribute_service.h"
#include "core/event_service.h"
#include "core/history_service.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"

namespace rt {

TimedDataImpl::TimedDataImpl(const scada::NodeId& node_id,
                             const TimedDataContext& context,
                             std::shared_ptr<const Logger> parent_logger)
    : TimedDataContext{context} {
  logger_.set_parent(std::move(parent_logger));
  logger_.set_prefix(node_id.ToString());

  SetNode(node_service_.GetNode(node_id));
}

TimedDataImpl::TimedDataImpl(const NodeRef& node,
                             const TimedDataContext& context,
                             std::shared_ptr<const Logger> parent_logger)
    : TimedDataContext{context} {
  logger_.set_parent(std::move(parent_logger));
  logger_.set_prefix(node.id().ToString());

  SetNode(node);
}

TimedDataImpl::~TimedDataImpl() {
  SetNode(nullptr);
}

void TimedDataImpl::SetNode(NodeRef node) {
  if (node_ == node)
    return;

  if (node_) {
    event_manager_.RemoveItemObserver(node_.id(), *this);
    node_.RemoveObserver(*this);
  }

  node_ = node;

  if (node_) {
    node_.AddObserver(*this);
    event_manager_.AddItemObserver(node_.id(), *this);

    alerting_ = event_manager_.IsAlerting(node_.id());

    monitored_value_ = realtime_service_.CreateMonitoredItem(node_.id(), scada::AttributeId::Value);
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
}

NodeRef TimedDataImpl::GetNode() const {
  return node_;
}

void TimedDataImpl::Write(double value, const scada::WriteFlags& flags,
                          const StatusCallback& callback) const {
  auto node = GetNode();
  if (!node) {
    if (callback)
      callback(scada::StatusCode::Bad_WrongNodeId);
    return;
  }
   
  attribute_service_.Write(node.id(), value, scada::NodeId(), flags, callback);
}

void TimedDataImpl::Call(const scada::NodeId& method_id, const std::vector<scada::Variant>& arguments,
                         const StatusCallback& callback) const {
  auto node = GetNode();
  if (!node)   {
    if (callback)
      callback(scada::StatusCode::Bad_WrongNodeId);
    return;
  }

  method_service_.Call(node.id(), method_id, arguments, callback);
}

void TimedDataImpl::HistoryRead() {
  assert(historical());

  if (from() >= ready_from()) {
    assert(from() == ready_from());
    return;
  }

  // Convert upper time range bound to query specific value:
  // |kTimedDataCurrentOnly| must be replaced on Time() in such case.
  base::Time to = ready_from();
  assert(!to.is_null());
  if (to == kTimedDataCurrentOnly)
    to = base::Time();

  logger_.WriteF(LogSeverity::Normal, "Querying history from {%s} to {%s}",
      FormatTime(from()).c_str(),
      !to.is_null() ? FormatTime(to).c_str() : "Current");

  querying_ = true;
  auto range = std::make_pair(from(), to);

  if (!node_) {
    OnQueryValuesComplete(scada::StatusCode::Bad, range.first, range.second, scada::QueryValuesResults());
    return;
  }

  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  history_service_.HistoryRead({node_.id(), scada::AttributeId::Value}, from(), to, {},
      io_service_.wrap([weak_ptr, range](scada::Status status, scada::QueryValuesResults values, scada::QueryEventsResults events) {
        if (auto ptr = weak_ptr.get())
          ptr->OnQueryValuesComplete(status, range.first, range.second, std::move(values));
      }));
}

void TimedDataImpl::OnFromChanged() {
  assert(historical());

  if (!querying_)
    HistoryRead();
}

std::string TimedDataImpl::GetFormula(bool aliases) const {
  return MakeNodeIdFormula(node_.id());
}

base::string16 TimedDataImpl::GetTitle() const {
  return base::SysNativeMBToWide(node_.display_name().text());
}

void TimedDataImpl::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  assert(node_id == node_.id());

  NotifyPropertyChanged(PropertySet(PROPERTY_TITLE | PROPERTY_CURRENT));

  // Notify specs.
  scada::PropertyIds property_ids;
  for (TimedDataSpecSet::iterator i = specs_.begin(); i != specs_.end(); ) {
    TimedDataSpec& spec = **i++;
    if (spec.delegate())
      spec.delegate()->OnTimedDataNodeModified(spec, property_ids);
  }
}

void TimedDataImpl::OnNodeDeleted(const scada::NodeId& node_id) {
  assert(node_id == node_.id());

  SetNode(nullptr);
  Delete();
}

void TimedDataImpl::OnItemEventsChanged(const scada::NodeId& item_id, const events::EventSet& events) {
  assert(item_id == node_.id());

  alerting_ = !events.empty();
  NotifyEventsChanged(events);
}

void TimedDataImpl::OnQueryValuesComplete(scada::Status status, base::Time queried_from, base::Time queried_to,
                                          scada::QueryValuesResults results) {
  assert(querying_);
  assert(queried_from < ready_from());
  assert(queried_from >= from());
  assert((!queried_to.is_null() && ready_from() == queried_to) ||
          (queried_to.is_null() && ready_from() == kTimedDataCurrentOnly));

  querying_ = false;

  // Merge requested data with existing realtime data by collection time.
  if (results) {
    for (auto& new_entry : *results)
      UpdateMap(new_entry);
  }

  base::Time new_ready_from = queried_from;
  // Returned data array includes left time bound.
  if (results && !results->empty() && results->front().source_timestamp < new_ready_from)
    new_ready_from = results->front().source_timestamp;

  UpdateReadyFrom(new_ready_from);

  // Query next fragment of data if |from()| changed from moment of last query.
  HistoryRead();

  // This probably may cause deletion.
  NotifyDataReady();
}

void TimedDataImpl::OnChannelData(const scada::DataValue& tvq) {
  if (tvq.qualifier.failed()) {
    monitored_value_.reset();
    Delete();
    return;
  }

  if (current_.source_timestamp.is_null() ||
      (!tvq.source_timestamp.is_null() && current_.source_timestamp <= tvq.source_timestamp)) {
    if (UpdateCurrent(tvq))
       NotifyPropertyChanged(PropertySet(PROPERTY_CURRENT));
  } else {
    if (UpdateMap(tvq))
      NotifyTimedDataCorrection(1, &tvq);
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

} // namespace rt