#pragma once

#include "base/memory/weak_ptr.h"
#include "common/event_observer.h"
#include "common/history_util.h"
#include "core/aggregate_filter.h"
#include "core/history_service.h"
#include "core/monitored_item.h"
#include "node_service/node_observer.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_context.h"

class TimedDataImpl : public std::enable_shared_from_this<TimedDataImpl>,
                      private TimedDataContext,
                      public BaseTimedData,
                      private NodeRefObserver,
                      private EventObserver {
 public:
  TimedDataImpl(NodeRef node,
                scada::AggregateFilter aggregate_filter,
                TimedDataContext context);
  ~TimedDataImpl();

  // TimedData overrides
  virtual std::string GetFormula(bool aliases) const override;
  virtual scada::LocalizedText GetTitle() const override;
  virtual NodeRef GetNode() const override;
  virtual void Write(double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const StatusCallback& callback) const override;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback) const override;
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

  base::WeakPtrFactory<TimedDataImpl> weak_ptr_factory_{this};
};
