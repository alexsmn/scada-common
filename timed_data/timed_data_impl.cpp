#include "timed_data/timed_data_impl.h"

#include "base/cancelation.h"
#include "base/debug_util.h"
#include "base/executor.h"
#include "common/formula_util.h"
#include "events/event_set.h"
#include "events/node_event_provider.h"
#include "model/node_id_util.h"
#include "timed_data/timed_data_fetcher.h"
#include "timed_data/timed_data_observer.h"

#include "base/debug_util-inl.h"

// TimedDataImpl

TimedDataImpl::TimedDataImpl(scada::AggregateFilter aggregate_filter,
                             TimedDataContext context)
    : TimedDataContext{std::move(context)},
      aggregate_filter_{std::move(aggregate_filter)},
      timed_data_fetcher_{
          services_.history_service
              ? std::make_shared<TimedDataFetcher>(TimedDataFetcherContext{
                    timed_data_view_, executor_, *services_.history_service,
                    aggregate_filter_})
              : nullptr} {}

TimedDataImpl::~TimedDataImpl() {
  SetNode(nullptr);
}

void TimedDataImpl::Init(NodeRef node) {
  assert(node);
  SetNode(std::move(node));

  scada::MonitoringParameters params;
  if (!aggregate_filter_.is_null())
    params.filter = aggregate_filter_;

  monitored_item_.subscribe_value(
      node_.scada_node(), params,
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

  if (timed_data_fetcher_) {
    timed_data_fetcher_->SetNode(node_);
  }
}

NodeRef TimedDataImpl::GetNode() const {
  return node_;
}

void TimedDataImpl::OnRangesChanged() {
  if (timed_data_fetcher_) {
    timed_data_fetcher_->FetchNextGap();
  }
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

void TimedDataImpl::OnChannelData(const scada::DataValue& data_value) {
  if (data_value.qualifier.failed()) {
    monitored_item_.unsubscribe();
    Delete();
    return;
  }

  if (IsUpdate(current_, data_value)) {
    if (UpdateCurrent(data_value)) {
      NotifyPropertyChanged(PropertySet(PROPERTY_CURRENT));
    }
  } else {
    if (timed_data_view_.InsertOrUpdate(data_value)) {
      timed_data_view_.NotifyUpdates({&data_value, 1});
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
