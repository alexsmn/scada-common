#pragma once

#include "common/history_util.h"
#include "events/event_observer.h"
#include "node_service/node_observer.h"
#include "scada/aggregate_filter.h"
#include "scada/history_service.h"
#include "scada/monitored_item.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_context.h"

class TimedDataImpl : public std::enable_shared_from_this<TimedDataImpl>,
                      private TimedDataContext,
                      public BaseTimedData,
                      private NodeRefObserver,
                      private EventObserver {
 public:
  TimedDataImpl(scada::AggregateFilter aggregate_filter,
                TimedDataContext context);
  ~TimedDataImpl();

  void Init(NodeRef node);

  // TimedData overrides
  virtual std::string GetFormula(bool aliases) const override;
  virtual scada::LocalizedText GetTitle() const override;
  virtual NodeRef GetNode() const override;
  virtual const EventSet* GetEvents() const override;
  virtual void Acknowledge() override;

 protected:
  // TimedData
  virtual void OnRangesChanged() override;

 private:
  void SetNode(const NodeRef& node);

  void FetchNextGap();
  void FetchMore(ScopedContinuationPoint continuation_point);

  void OnHistoryReadRawComplete(std::vector<scada::DataValue> values,
                                ScopedContinuationPoint continuation_point);

  void OnChannelData(const scada::DataValue& data_value);

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;

  // EventObserver
  virtual void OnItemEventsChanged(const scada::NodeId& node_id,
                                   const EventSet& events) override;

  const scada::AggregateFilter aggregate_filter_;

  NodeRef node_;
  std::shared_ptr<scada::MonitoredItem> monitored_value_;

  bool querying_ = false;
  scada::DateTimeRange querying_range_;
};
